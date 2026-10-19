#!/usr/bin/env bash
#
# Build a signed release APK and publish it in App/dist/ so the app can offer
# the update (same idea as Firmware/tools/release.sh + latest.json).
#
# Usage:
#   App/android/tools/release.sh 1.1        # set version 1.1 and build
#   App/android/tools/release.sh            # rebuild the current version
#
# It writes:
#   App/dist/petalot.apk                    signed APK, stable name (links never change)
#   App/dist/app-latest.json                manifest the app checks
#
# The stable file name is deliberate: the website and the READMEs can link to
# App/dist/petalot.apk forever, and the version is read from the manifest.
# Older builds stay reachable through git history.
#
# The manifest and the APK are meant to be committed: the app reads them from
# raw.githubusercontent.com, so no server of your own is involved.
#
# Commit everything together:
#   git add App/dist App/android/app/build.gradle.kts
#   git commit -m "app: v<version>"
#   git push

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ANDROID_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
REPO_DIR="$(cd "$ANDROID_DIR/../.." && pwd)"
DIST="$REPO_DIR/App/dist"
GRADLE="$ANDROID_DIR/app/build.gradle.kts"
BASE_URL="https://raw.githubusercontent.com/function3d/petalot/master/App/dist/"

current_name="$(sed -n 's/^ *versionName = "\(.*\)"/\1/p' "$GRADLE")"
version="${1:-$current_name}"

if ! printf '%s' "$version" | grep -Eq '^[0-9]+\.[0-9]+(\.[0-9]+)?$'; then
  echo "error: version must look like 1.1 or 1.1.0" >&2
  exit 1
fi

major="${version%%.*}"
rest="${version#*.}"
minor="${rest%%.*}"
if [ "$rest" = "$minor" ]; then patch=0; else patch="${rest#*.}"; fi
# versionCode must grow on every release; derive it from the version name.
code=$((major * 10000 + minor * 100 + patch))

if [ -n "${1:-}" ]; then
  sed -i "s/^ *versionName = \".*\"/    versionName = \"$version\"/" "$GRADLE"
  sed -i "s/^ *versionCode = [0-9]*/        versionCode = $code/" "$GRADLE"
  echo "==> version $version (versionCode $code)"
fi

if [ ! -f "$ANDROID_DIR/keystore.properties" ]; then
  echo "error: $ANDROID_DIR/keystore.properties missing; cannot sign" >&2
  exit 1
fi

echo "==> building signed release"
(cd "$ANDROID_DIR" && ./gradlew assembleRelease --console=plain -q)

APK="$ANDROID_DIR/app/build/outputs/apk/release/app-release.apk"
[ -f "$APK" ] || { echo "error: release APK not built" >&2; exit 1; }

mkdir -p "$DIST"
# One stable file name, so links never change: the version lives in the
# manifest and in git history.
rm -f "$DIST"/petalot.apk "$DIST"/petalot-*.apk
out="$DIST/petalot.apk"
cp "$APK" "$out"

sha="$(sha256sum "$out" | cut -d' ' -f1)"
size="$(stat -c%s "$out")"

python3 - "$DIST/app-latest.json" "$code" "$version" "$sha" "$size" "$BASE_URL" <<'PY'
import json, sys
path, code, name, sha, size, base = sys.argv[1:7]
file = "petalot.apk"
manifest = {
    "versionCode": int(code),
    "versionName": name,
    "file": file,
    # The query busts GitHub's CDN cache; the file name never changes.
    "url": "%s%s?v=%s" % (base, file, name),
    "sha256": sha,
    "size": int(size),
}
with open(path, "w") as f:
    json.dump(manifest, f, indent=2)
    f.write("\n")
print(json.dumps(manifest, indent=2))
PY

kb=$(( (size + 1023) / 1024 ))
echo "==> updating README download links"
for f in "$REPO_DIR/README.md" "$REPO_DIR/README_es.md" "$REPO_DIR/README_pt.md"; do
  [ -f "$f" ] || continue
  sed -i -E \
    -e "s#App/dist/petalot(-[0-9]+(\.[0-9]+)?)?\.apk#App/dist/petalot.apk#g" \
    -e "s#\([0-9]+ KB\) — Android 7\.0#($kb KB) — Android 7.0#g" \
    "$f"
done

echo
echo "Built PETALOT app $version (versionCode $code)."
echo "Next:"
echo "  git add App/dist App/android/app/build.gradle.kts README.md README_es.md README_pt.md"
echo "  git commit -m \"app: v$version\""
echo "  git push"
