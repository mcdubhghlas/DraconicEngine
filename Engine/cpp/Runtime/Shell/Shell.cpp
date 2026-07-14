// The shell factory bridge. createShell()/destroyShell() dispatch to the backend
// chosen at compile time: DRACO_SHELL_SDL3 selects the SDL3 desktop backend, otherwise
// the headless null backend. Both backends are compiled into this one Shell library;
// the bridge decides which one createShell() builds.
module shell;

#if DRACO_SHELL_SDL3
import shell.desktop;
#else
import shell.null;
#endif

namespace draco::shell
{
    IShell* createShell(const WindowSettings& settings)
    {
#if DRACO_SHELL_SDL3
        return new SDL3Shell(settings);
#else
        return new NullShell(settings);
#endif
    }

    void destroyShell(IShell* shell) noexcept
    {
        delete shell;
    }
}
