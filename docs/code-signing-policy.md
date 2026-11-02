# Code signing

The MX400 Controller installers are **not code signed**. We do not pay for certificates and we are not using a free signing service for now.

What you will see:
- **Windows:** SmartScreen may show "Windows protected your PC". Click More info, then Run anyway.
- **macOS:** a warning the first time you open the installer. See [macos-install.md](macos-install.md).
- **Linux:** no such step.

## How to check a download yourself
- The full source code is public and the installers are built by public GitHub Actions from it.
- Every release has `SHA256SUMS` inside the zip. Check your files against it (`sha256sum -c SHA256SUMS` on Linux, `shasum -a 256 -c SHA256SUMS` on macOS).
- The plugin makes no network connections and collects nothing.
