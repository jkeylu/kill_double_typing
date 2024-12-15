// alterkeys.c
// http://osxbook.com
//
// You need superuser privileges to create the event tap, unless accessibility
// is enabled. To do so, select the "Enable access for assistive devices"
// checkbox in the Universal Access system preference pane.

// modified by SF-Zhou
// To: Kill Double Typing on MacBook
// Complile: g++ -std=c++11 -O2 -Wall -o kill_double_typing kill_double_typing.cpp -framework ApplicationServices
// Run: ./kill_double_typing --delay-all-keys --default-delay-duration 40 --delay-key n:65 --delay-key j:60 --delay-key x:50

// Code from the article: https://sf-zhou.github.io/productivity/solve_macbook_typing_double.html
// 2024-10-20 modified by jKey Lu (https://github.com/jkeylu)

#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <utility>
#include <sstream>
#include <iomanip>


typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
typedef long long ll;

TimePoint time_now();
std::string get_current_time_string(const std::string& format);
void init_config(int argc, char* argv[]);
void print_config();

// http://ritter.ist.psu.edu/projects/RUI/macosx/rui.c
const char * keyStringForKeyCode(int keyCode); // get the RUI representation of the Mac keycode
int keyCodeForKeyString(const char * keyString); // get the Mac keycode for the RUI representation

int delay_all_keys = 0;
ll default_delay_duration = 40 * 1000;

std::unordered_map<CGKeyCode, ll> delay_duration_map;
std::unordered_map<CGKeyCode, TimePoint> last_time_map;


// This callback will be invoked every time there is a keystroke.
CGEventRef myCGEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon) {
  // Paranoid sanity check.
  if ((type != kCGEventKeyDown) && (type != kCGEventKeyUp)) {
    return event;
  }

  TimePoint now = time_now();

  // The incoming keycode.
  CGKeyCode keycode = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
  if (!delay_all_keys && !delay_duration_map.count(keycode)) {
    return event;
  }

  if (type == kCGEventKeyUp) {
    last_time_map[keycode] = now;

  } else if (last_time_map.count(keycode)) {
    ll microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now - last_time_map[keycode]).count();

    ll delay_duration = default_delay_duration;
    if (delay_duration_map.count(keycode)) {
      delay_duration = delay_duration_map[keycode];
    }

    // ignore if time less than delay_duration
    if (microseconds < delay_duration) {
      std::cout << get_current_time_string("%Y-%m-%d %H:%M:%S")
        << " - keycode: " << keycode
        << ", char: " << keyStringForKeyCode(keycode)
        << ", duration: " << (int) microseconds / 1000 << "ms"
        << std::endl;
      return NULL;
    }
  }

  // We must return the event for it to be useful.
  return event;
}

int main(int argc, char* argv[]) {
	init_config(argc, argv);
  print_config();

  CFMachPortRef      eventTap;
  CGEventMask        eventMask;
  CFRunLoopSourceRef runLoopSource;

  // Create an event tap. We are interested in key presses.
  eventMask = ((1 << kCGEventKeyDown) | (1 << kCGEventKeyUp));
  eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, (CGEventTapOptions)0, eventMask, myCGEventCallback, NULL);
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

TimePoint time_now() {
  return std::chrono::high_resolution_clock::now();
}

std::string get_current_time_string(const std::string& format) {
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 转换为time_t以便与C语言兼容
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // 转换为本地时间
    std::tm* local_time = std::localtime(&now_time_t);

    // 使用字符串流进行格式化
    std::ostringstream ss;
    ss << std::put_time(local_time, format.c_str());

    // 返回格式化的时间字符串
    return ss.str();
}

void init_config(int argc, char* argv[]) {
  std::vector<std::pair<CGKeyCode, ll> > delay_key_list;
  int print_usage = 0;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "--delay-all-keys") {
      delay_all_keys = 1;

    } else if (arg == "--default-delay-duration") {
      if (i + 1 >= argc) {
        std::cerr << "--default-delay-duration require a value" << std::endl;
        exit(1);
      }

      try {
        default_delay_duration = std::stoll(argv[i + 1]) * 1000;
      } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid argument: \"" << arg << " " << argv[i + 1] << "\"" << std::endl;
        exit(1);
      } catch (const std::out_of_range& e) {
        std::cerr << "Out of range: \"" << arg << " " << argv[i + 1] << "\"" << std::endl;
        exit(1);
      }

      i++;

    } else if (arg == "--delay-key") {
      if (i + 1 >= argc) {
        std::cerr << "--delay-key require a value" << std::endl;
        exit(1);
      }

      std::string val = argv[i + 1];

      size_t pos = val.find(':');
      if (pos == std::string::npos) {
        delay_key_list.push_back(std::make_pair(keyCodeForKeyString(val.c_str()), -1));
      } else {
        std::string key = val.substr(0, pos);
        ll delay_duration = 0;
        try {
          delay_duration = std::stoll(val.substr(pos + 1));
        } catch (const std::invalid_argument& e) {
          std::cerr << "Invalid argument: \"" << arg << " " << val << "\"" << std::endl;
          exit(1);
        } catch (const std::out_of_range& e) {
          std::cerr << "Out of range \"" << arg << " " << val << "\"" << std::endl;
          exit(1);
        }
        delay_key_list.push_back(std::make_pair(keyCodeForKeyString(key.c_str()), delay_duration));
      }

      i += 1;

    } else {
      std::cerr << "Invalid argument: \"" << arg << "\"" << std::endl;
      print_usage = 1;
      break;
    }
  }

  if (argc == 1 || print_usage) {
    std::cerr << "Usage example: " << argv[0] << " --delay-all-keys --default-delay-duration 40 --delay-key n:65 --delay-key j:60 --delay-key x:50" << std::endl;
	std::cerr << "Read code: https://github.com/jkeylu/kill_double_typing" << std::endl;
    exit(1);
  }

  for (const auto& item : delay_key_list) {
    ll delay_duration = item.second;
    if (delay_duration <= 0) {
      delay_duration = default_delay_duration;
    } else {
      delay_duration = delay_duration * 1000;
    }
    delay_duration_map[item.first] = delay_duration;
  }
}

void print_config() {
  std::cout << "/**" << std::endl;
  std::cout << " * delay_all_keys = " << delay_all_keys << std::endl;
  std::cout << " * default_delay_duration = " << default_delay_duration / 1000 << "ms" << std::endl;
  for (const auto& pair : delay_duration_map) {
    std::cout << " * delay_key = " << keyStringForKeyCode(pair.first) << ":" << pair.second / 1000 << "ms" << std::endl;
  }
  std::cout << " */" << std::endl;
}

const char * keyStringForKeyCode(int keyCode) {
	// Proper key detection seems to want a switch statement, unfortunately
	switch (keyCode) {
		case 0: return("a");
		case 1: return("s");
		case 2: return("d");
		case 3: return("f");
		case 4: return("h");
		case 5: return("g");
		case 6: return("z");
		case 7: return("x");
		case 8: return("c");
		case 9: return("v");
		// what is 10?
		case 11: return("b");
		case 12: return("q");
		case 13: return("w");
		case 14: return("e");
		case 15: return("r");
		case 16: return("y");
		case 17: return("t");
		case 18: return("1");
		case 19: return("2");
		case 20: return("3");
		case 21: return("4");
		case 22: return("6");
		case 23: return("5");
		case 24: return("=");
		case 25: return("9");
		case 26: return("7");
		case 27: return("-");
		case 28: return("8");
		case 29: return("0");
		case 30: return("]");
		case 31: return("o");
		case 32: return("u");
		case 33: return("[");
		case 34: return("i");
		case 35: return("p");
		case 36: return("RETURN");
		case 37: return("l");
		case 38: return("j");
		case 39: return("'");
		case 40: return("k");
		case 41: return(";");
		case 42: return("\\");
		case 43: return(",");
		case 44: return("/");
		case 45: return("n");
		case 46: return("m");
		case 47: return(".");
		case 48: return("TAB");
		case 49: return("SPACE");
		case 50: return("`");
		case 51: return("DELETE");
		case 52: return("ENTER");
		case 53: return("ESCAPE");

		// some more missing codes abound, reserved I presume, but it would
		// have been helpful for Apple to have a document with them all listed

		case 65: return(".");

		case 67: return("*");

		case 69: return("+");

		case 71: return("CLEAR");

		case 75: return("/");
		case 76: return("ENTER");   // numberpad on full kbd

		case 78: return("-");

		case 81: return("=");
		case 82: return("0");
		case 83: return("1");
		case 84: return("2");
		case 85: return("3");
		case 86: return("4");
		case 87: return("5");
		case 88: return("6");
		case 89: return("7");

		case 91: return("8");
		case 92: return("9");

		case 96: return("F5");
		case 97: return("F6");
		case 98: return("F7");
		case 99: return("F3");
		case 100: return("F8");
		case 101: return("F9");

		case 103: return("F11");

		case 105: return("F13");

		case 107: return("F14");

		case 109: return("F10");

		case 111: return("F12");

		case 113: return("F15");
		case 114: return("HELP");
		case 115: return("HOME");
		case 116: return("PGUP");
		case 117: return("DELETE");  // full keyboard right side numberpad
		case 118: return("F4");
		case 119: return("END");
		case 120: return("F2");
		case 121: return("PGDN");
		case 122: return("F1");
		case 123: return("LEFT");
		case 124: return("RIGHT");
		case 125: return("DOWN");
		case 126: return("UP");

		default:
			// Unknown key, bail and note that RUI needs improvement
			// fprintf(stderr, "%ld\tKey\t%c (DEBUG: %d)\n", currenttime, keyCode);
			// exit(EXIT_FAILURE);
      return("Unknown key");
	}
}

int keyCodeForKeyString(const char * keyString) {
	if (strcmp(keyString, "a") == 0) return 0;
	if (strcmp(keyString, "s") == 0) return 1;
	if (strcmp(keyString, "d") == 0) return 2;
	if (strcmp(keyString, "f") == 0) return 3;
	if (strcmp(keyString, "h") == 0) return 4;
	if (strcmp(keyString, "g") == 0) return 5;
	if (strcmp(keyString, "z") == 0) return 6;
	if (strcmp(keyString, "x") == 0) return 7;
	if (strcmp(keyString, "c") == 0) return 8;
	if (strcmp(keyString, "v") == 0) return 9;
	// what is 10?
	if (strcmp(keyString, "b") == 0) return 11;
	if (strcmp(keyString, "q") == 0) return 12;
	if (strcmp(keyString, "w") == 0) return 13;
	if (strcmp(keyString, "e") == 0) return 14;
	if (strcmp(keyString, "r") == 0) return 15;
	if (strcmp(keyString, "y") == 0) return 16;
	if (strcmp(keyString, "t") == 0) return 17;
	if (strcmp(keyString, "1") == 0) return 18;
	if (strcmp(keyString, "2") == 0) return 19;
	if (strcmp(keyString, "3") == 0) return 20;
	if (strcmp(keyString, "4") == 0) return 21;
	if (strcmp(keyString, "6") == 0) return 22;
	if (strcmp(keyString, "5") == 0) return 23;
	if (strcmp(keyString, "=") == 0) return 24;
	if (strcmp(keyString, "9") == 0) return 25;
	if (strcmp(keyString, "7") == 0) return 26;
	if (strcmp(keyString, "-") == 0) return 27;
	if (strcmp(keyString, "8") == 0) return 28;
	if (strcmp(keyString, "0") == 0) return 29;
	if (strcmp(keyString, "]") == 0) return 30;
	if (strcmp(keyString, "o") == 0) return 31;
	if (strcmp(keyString, "u") == 0) return 32;
	if (strcmp(keyString, "[") == 0) return 33;
	if (strcmp(keyString, "i") == 0) return 34;
	if (strcmp(keyString, "p") == 0) return 35;
	if (strcmp(keyString, "RETURN") == 0) return 36;
	if (strcmp(keyString, "l") == 0) return 37;
	if (strcmp(keyString, "j") == 0) return 38;
	if (strcmp(keyString, "'") == 0) return 39;
	if (strcmp(keyString, "k") == 0) return 40;
	if (strcmp(keyString, ";") == 0) return 41;
	if (strcmp(keyString, "\\") == 0) return 42;
	if (strcmp(keyString, ",") == 0) return 43;
	if (strcmp(keyString, "/") == 0) return 44;
	if (strcmp(keyString, "n") == 0) return 45;
	if (strcmp(keyString, "m") == 0) return 46;
	if (strcmp(keyString, ".") == 0) return 47;
	if (strcmp(keyString, "TAB") == 0) return 48;
	if (strcmp(keyString, "SPACE") == 0) return 49;
	if (strcmp(keyString, "`") == 0) return 50;
	if (strcmp(keyString, "DELETE") == 0) return 51;
	if (strcmp(keyString, "ENTER") == 0) return 52;
	if (strcmp(keyString, "ESCAPE") == 0) return 53;

	// some more missing codes abound, reserved I presume, but it would
	// have been helpful for Apple to have a document with them all listed

	if (strcmp(keyString, ".") == 0) return 65;

	if (strcmp(keyString, "*") == 0) return 67;

	if (strcmp(keyString, "+") == 0) return 69;

	if (strcmp(keyString, "CLEAR") == 0) return 71;

	if (strcmp(keyString, "/") == 0) return 75;
	if (strcmp(keyString, "ENTER") == 0) return 76;  // numberpad on full kbd

	if (strcmp(keyString, "=") == 0) return 78;

	if (strcmp(keyString, "=") == 0) return 81;
	if (strcmp(keyString, "0") == 0) return 82;
	if (strcmp(keyString, "1") == 0) return 83;
	if (strcmp(keyString, "2") == 0) return 84;
	if (strcmp(keyString, "3") == 0) return 85;
	if (strcmp(keyString, "4") == 0) return 86;
	if (strcmp(keyString, "5") == 0) return 87;
	if (strcmp(keyString, "6") == 0) return 88;
	if (strcmp(keyString, "7") == 0) return 89;

	if (strcmp(keyString, "8") == 0) return 91;
	if (strcmp(keyString, "9") == 0) return 92;

	if (strcmp(keyString, "F5") == 0) return 96;
	if (strcmp(keyString, "F6") == 0) return 97;
	if (strcmp(keyString, "F7") == 0) return 98;
	if (strcmp(keyString, "F3") == 0) return 99;
	if (strcmp(keyString, "F8") == 0) return 100;
	if (strcmp(keyString, "F9") == 0) return 101;

	if (strcmp(keyString, "F11") == 0) return 103;

	if (strcmp(keyString, "F13") == 0) return 105;

	if (strcmp(keyString, "F14") == 0) return 107;

	if (strcmp(keyString, "F10") == 0) return 109;

	if (strcmp(keyString, "F12") == 0) return 111;

	if (strcmp(keyString, "F15") == 0) return 113;
	if (strcmp(keyString, "HELP") == 0) return 114;
	if (strcmp(keyString, "HOME") == 0) return 115;
	if (strcmp(keyString, "PGUP") == 0) return 116;
	if (strcmp(keyString, "DELETE") == 0) return 117;
	if (strcmp(keyString, "F4") == 0) return 118;
	if (strcmp(keyString, "END") == 0) return 119;
	if (strcmp(keyString, "F2") == 0) return 120;
	if (strcmp(keyString, "PGDN") == 0) return 121;
	if (strcmp(keyString, "F1") == 0) return 122;
	if (strcmp(keyString, "LEFT") == 0) return 123;
	if (strcmp(keyString, "RIGHT") == 0) return 124;
	if (strcmp(keyString, "DOWN") == 0) return 125;
	if (strcmp(keyString, "UP") == 0) return 126;

	// fprintf(stderr, "keyString %s Not Found. Aborting...\n", keyString);
	// exit(EXIT_FAILURE);
  return -1;
}
