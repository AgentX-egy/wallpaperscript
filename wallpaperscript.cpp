#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include "USAGE_MESSAGE.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <combaseapi.h>
#include <shobjidl.h>

using dispaly_wallpaper = std::pair<int, std::filesystem::path>;
using std::string_view_literals::operator""sv;

class DesktopWallpaperWrapper {
public:
  explicit DesktopWallpaperWrapper() {
    HRESULT hr = CoCreateInstance(CLSID_DesktopWallpaper, nullptr,
                                  CLSCTX_LOCAL_SERVER, IID_IDesktopWallpaper,
                                  // static_cast<void **>(&desktop_wallpaper));
                                  (void **)&desktop_wallpaper);
    p_good = SUCCEEDED(hr);
  }
  ~DesktopWallpaperWrapper() {
    p_good = false;
    desktop_wallpaper->Release();
  }
  bool good() { return p_good; }

public:
  CONST UINT attached_monitor_count() {
    HRESULT hr;
    UINT attached_monitor_count = 0;
    do {
      wchar_t *monitor_id;
      desktop_wallpaper->GetMonitorDevicePathAt(attached_monitor_count,
                                                &monitor_id);

      RECT test;
      hr = desktop_wallpaper->GetMonitorRECT(monitor_id, &test);
    } while (SUCCEEDED(hr) && hr != S_FALSE);
    return attached_monitor_count;
  }

  void set_all_wallpapers(const std::filesystem::path &path) {
    desktop_wallpaper->SetWallpaper(nullptr, path.c_str());
  }

  void set_wallpaper(UINT monitor_index, const std::filesystem::path &path) {
    wchar_t *monitor_id;
    desktop_wallpaper->GetMonitorDevicePathAt(monitor_index, &monitor_id);
    RECT test;
    HRESULT hr = desktop_wallpaper->GetMonitorRECT(monitor_id, &test);
    if (SUCCEEDED(hr) && S_FALSE != hr)
      desktop_wallpaper->SetWallpaper(monitor_id, path.c_str());
  }

  // unsafe?
  void set_wallpaper_fast(UINT monitor_index,
                          const std::filesystem::path &path) {
    wchar_t *monitor_id;
    desktop_wallpaper->GetMonitorDevicePathAt(monitor_index, &monitor_id);
    desktop_wallpaper->SetWallpaper(monitor_id, path.c_str());
  }

private:
  IDesktopWallpaper *desktop_wallpaper = nullptr;
  bool p_good = false;
};

bool isValidPath(const std::wstring_view arg) {
  std::error_code err;
  auto f_status = std::filesystem::status(arg, err);
  if (!err && (f_status.type() == std::filesystem::file_type::regular ||
               f_status.type() == std::filesystem::file_type::symlink)) {
    return true;
  } else {
    std::cerr << err.message();
    return false;
  }
}

bool isDigit(const std::wstring_view arg) {
  return arg.find_first_not_of(L"0123456789") == std::wstring_view::npos;
}

auto parse_multiple(const std::vector<std::wstring_view> &argv,
                    CONST UINT max_monitors = 99) {
  UINT enum_counter = 0;

  std::vector<dispaly_wallpaper> inputs;
  inputs.reserve((argv.size() < max_monitors) ? argv.size() : max_monitors);

  for (const auto arg : argv) {
    if (enum_counter > max_monitors) {
      break;
    }
    if (arg.compare(L"N"sv) && isValidPath(arg)) {
      inputs.emplace_back(enum_counter, arg);
    }
    enum_counter++;
  }

  return inputs;
}

inline enum class ProgramOptions { single, all, multiple, noop } program_option;
ProgramOptions parse_option(int argc, wchar_t **&argv) {
  if (argc > 2) {
    std::wstring_view control{argv[1]};
    if (isDigit(control) && isValidPath(argv[2])) {
      return ProgramOptions::single;
    } else if (control.compare(L"/A") == 0 && isValidPath(argv[2])) {
      return ProgramOptions::all;
    } else if (control.compare(L"/M") == 0) {
      return ProgramOptions::multiple;
    }
  }
  return ProgramOptions::noop;
}

int wmain(int argc, wchar_t **argv) {
  ProgramOptions program_option = parse_option(argc, argv);

  // print USAGE
  if (program_option == ProgramOptions::noop) {
    std::wcout << UsageMessage << std::endl;
    return 0;
  }

  HRESULT hr =
      CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  if (SUCCEEDED(hr)) {
    if (DesktopWallpaperWrapper dww; dww.good()) {
      std::vector<std::wstring_view> argv_vector;
      switch (program_option) {
      case ProgramOptions::single: {
        dww.set_wallpaper(std::stoi(argv[1]), argv[2]);
      } break;
      case ProgramOptions::all: {
        dww.set_all_wallpapers(argv[2]);
      } break;
      case ProgramOptions::multiple: {
        auto v = parse_multiple(argv_vector, dww.attached_monitor_count());
        for (const auto &[dispaly, path] : v) {
          dww.set_wallpaper_fast(dispaly, path);
        }
      } break;
      // Apeasing the Clang
      case ProgramOptions::noop: {
      } break;
      };
    } else {
      std::wcerr << L"Failed to initialize IDesktopWallpaper";
    }
    CoUninitialize();
    return 0;
  } else {
    std::wcerr << L"Failed to initialize COM";
  }
  return 1;
}