#!/usr/bin/env bash
set -euo pipefail

# ---- defaults (edit if you want) ----
DEFAULT_APP="cmake-build-debug/bitKlavier0_artefacts/Debug/Standalone/bitKlavier0.app"
ENTITLEMENTS="/tmp/instruments.entitlements.plist"

# ---- args ----
# Usage:
#   ./scripts/sign_for_instruments.sh                # uses DEFAULT_APP relative to repo root
#   ./scripts/sign_for_instruments.sh /path/to/app   # uses provided .app path
APP_PATH="${1:-}"

# ---- resolve repo root (folder containing this script -> parent) ----
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

if [[ -z "$APP_PATH" ]]; then
  APP_PATH="$REPO_ROOT/$DEFAULT_APP"
fi

echo "Signing for Instruments:"
echo "  APP: $APP_PATH"

# ---- write entitlements ----
cat >"$ENTITLEMENTS" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>com.apple.security.get-task-allow</key>
  <true/>
</dict>
</plist>
PLIST

# ---- sign (ad-hoc) with debuggable entitlement ----
/usr/bin/codesign --force --deep --sign - --entitlements "$ENTITLEMENTS" "$APP_PATH"

# ---- verify + print summary ----
echo ""
echo "codesign summary:"
/usr/bin/codesign -dv --verbose=4 "$APP_PATH" 2>&1 | sed -n '1,60p'

echo ""
echo "entitlements:"
/usr/bin/codesign -d --entitlements :- "$APP_PATH" || true

echo ""
echo "verification:"
/usr/bin/codesign --verify --deep --strict --verbose=2 "$APP_PATH"

echo ""
echo "✅ Done. Relaunch the app, then attach with Instruments."
