#include "../src/AuthorizationActivity.h"

#include <cassert>
#include <iostream>

AuthorizationActivity::Entry activity(uint32_t id, uint32_t index, const std::string& name = "Thomas")
{
    AuthorizationActivity::Entry entry;
    entry.authorizationId = id;
    entry.index = index;
    entry.name = name;
    entry.dateLastActive = "2026-10-22 09:30:00";
    entry.action = "Lock";
    entry.trigger = "system";
    entry.completionStatus = "success";
    return entry;
}

void newestActivitySurvivesReplay()
{
    AuthorizationActivity tracker;
    auto newest = activity(42, 100);
    newest.action = "Unlock";
    assert(tracker.observe(newest, 10));
    assert(!tracker.listReceived());

    assert(!tracker.observe(activity(42, 99, "Older name"), 10));
    assert(!tracker.observe(activity(42, 100, "Duplicate name"), 10));
    assert(tracker.entries().at(42).name == "Thomas");
    assert(tracker.entries().at(42).action == "Unlock");
    assert(tracker.entries().at(42).index == 100);

    auto next = activity(42, 101, "New name");
    next.dateLastActive = "2026-10-22 09:35:00";
    next.completionStatus = "motorBlocked";
    assert(tracker.observe(next, 10));
    const auto& saved = tracker.entries().at(42);
    assert(saved.authorizationId == 42);
    assert(saved.index == 101);
    assert(saved.name == "New name");
    assert(saved.dateLastActive == "2026-10-22 09:35:00");
    assert(saved.action == "Lock");
    assert(saved.trigger == "system");
    assert(saved.completionStatus == "motorBlocked");

    assert(tracker.observe(activity(43, 90), 10));
    assert(tracker.entries().at(43).index == 90);
}

void rejectsUnattributedAndInvalidActivity()
{
    AuthorizationActivity tracker;
    assert(!tracker.observe(activity(0, 1), 10));
    assert(!tracker.observe(activity(std::numeric_limits<uint32_t>::max(), 1), 10));
    assert(!tracker.observe(activity(42, 0), 10));
    assert(!tracker.observe(activity(42, 1, ""), 10));
    assert(!tracker.observe(activity(42, 1, " \t\r\n"), 10));
    assert(!tracker.observe(activity(42, 1), 0));
    assert(tracker.entries().empty());

    const std::string fullName(32, 'x');
    assert(tracker.observe(activity(42, 1, fullName), 10));
    assert(tracker.entries().at(42).name == fullName);
}

void capacityDoesNotEvictExistingSensors()
{
    AuthorizationActivity tracker;
    assert(tracker.observe(activity(42, 1), 2));
    assert(tracker.observe(activity(43, 2), 2));
    assert(!tracker.observe(activity(44, 3), 2));
    assert(tracker.entries().size() == 2);
    assert(tracker.entries().count(42) == 1);
    assert(tracker.observe(activity(42, 4), 2));
    assert(tracker.entries().at(42).index == 4);

    tracker.updateListed({43});
    assert(tracker.observe(activity(44, 5), 2));
    assert(tracker.entries().size() == 2);
}

void listedAuthorizationsReplaceFallbacks()
{
    AuthorizationActivity tracker;
    assert(tracker.observe(activity(42, 1), 10));
    assert(tracker.observe(activity(43, 2), 10));

    tracker.updateListed({42});
    assert(tracker.listReceived());
    assert(tracker.entries().count(42) == 0);
    assert(tracker.entries().count(43) == 1);
    assert(!tracker.observe(activity(42, 3), 10));

    tracker.updateListed({43});
    assert(tracker.entries().empty());
    assert(tracker.observe(activity(42, 4), 10));
    assert(!tracker.observe(activity(43, 5), 10));

    tracker.clear();
    assert(!tracker.listReceived());
    assert(tracker.entries().empty());
    assert(tracker.observe(activity(43, 1), 10));
    tracker.updateListed({});
    assert(tracker.listReceived());
    assert(tracker.entries().count(43) == 1);
}

int main()
{
    newestActivitySurvivesReplay();
    rejectsUnattributedAndInvalidActivity();
    capacityDoesNotEvictExistingSensors();
    listedAuthorizationsReplaceFallbacks();
    std::cout << "Authorization activity tests passed\n";
}
