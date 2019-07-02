#include <ndsl/framework/all.hpp>

using ndsl::framework::Event;

bool ndsl::framework::operator==(const Event &ev1, const Event &ev2) {
  if (ev1.event_type_id() == ev2.event_type_id() &&
      ev1.event_id() == ev2.event_id()) {
    return true;
  }

  return false;
}

bool ndsl::framework::operator>(const Event &ev1, const Event &ev2) {
  if (ev1.event_type_id() > ev2.event_type_id()) {
    return true;
  } else if (ev1.event_type_id() == ev2.event_type_id() &&
             ev1.event_id() > ev2.event_id()) {
    return true;
  }

  return false;
}

bool ndsl::framework::operator<(const Event &ev1, const Event &ev2) {
  if (ev1.event_type_id() < ev2.event_type_id()) {
    return true;
  } else if (ev1.event_type_id() == ev2.event_type_id() &&
             ev1.event_id() < ev2.event_id()) {
    return true;
  }

  return false;
}

bool ndsl::framework::operator>=(const Event &ev1, const Event &ev2) {
  if (ev1 < ev2) {
    return false;
  }

  return true;
}

bool ndsl::framework::operator<=(const Event &ev1, const Event &ev2) {
  if (ev1 > ev2) {
    return false;
  }

  return true;
}
