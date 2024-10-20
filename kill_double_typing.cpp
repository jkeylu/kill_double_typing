// alterkeys.c
// http://osxbook.com
//
// You need superuser privileges to create the event tap, unless accessibility
// is enabled. To do so, select the "Enable access for assistive devices"
// checkbox in the Universal Access system preference pane.

// modified by SF-Zhou
// To: Kill Double Typing on MacBook
// Complile: g++ -O2 -Wall -o kill_double_typing kill_double_typing.cpp -framework ApplicationServices
// Run: ./kill_double_typing

#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <chrono>
#include <unordered_map>
using namespace std;

typedef chrono::time_point<std::chrono::high_resolution_clock> Time;
typedef long long ll;

unordered_map<CGKeyCode, Time> last_time;

Time time_now() {
  return chrono::high_resolution_clock::now();
}

// This callback will be invoked every time there is a keystroke.
CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
  // Paranoid sanity check.
  if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp)) {
    return event;
  }

  // The incoming keycode.
  CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);

  // printf("%d\n", keycode);  // print keycode
  if (keycode == 11 /* b */ || keycode == 45 /* n */) {
    if (type == kCGEventKeyUp) {
      last_time[keycode] = time_now();
    } else {
      if (last_time.count(keycode)) {
        ll microseconds = chrono::duration_cast<chrono::microseconds>(
          time_now() - last_time[keycode]
        ).count();

        // ignore if time less than 30ms
        if (microseconds < 30000) {
          return NULL;
        }
      }
    }
  }

  // Set the modified keycode field in the event.
  CGEventSetIntegerValueField(event, kCGKeyboardEventKeycode, (int64_t)keycode);

  // We must return the event for it to be useful.
  return event;
}

int main(void) {
  CFMachPortRef      eventTap;
  CGEventMask        eventMask;
  CFRunLoopSourceRef runLoopSource;

  // Create an event tap. We are interested in key presses.
  eventMask = ((1 << kCGEventKeyDown) | (1 << kCGEventKeyUp));
  eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, 0, eventMask, myCGEventCallback, NULL);
  if (!eventTap) {
      fprintf(stderr, "failed to create event tap\n");
      exit(1);
  }

  // Create a run loop source.
  runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);

  // Add to the current run loop.
  CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

  // Enable the event tap.
  CGEventTapEnable(eventTap, true);

  // Set it all running.
  CFRunLoopRun();

  // In a real program, one would have arranged for cleaning up.

  exit(0);
}