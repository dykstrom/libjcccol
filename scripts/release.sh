#!/usr/bin/env bash
# release.sh — bump VERSION, commit, and tag a new libjcccol release.
#
# The push (which triggers the CI release workflow) is left to the user;
# this script only prepares the local commit and tag, so you can review
# `git log -1` / `git show` before publishing.
#
# Platform: macOS and Linux. Bash 3.2+ (macOS default) is sufficient.
# Windows is not supported by this script — Windows users who need to cut
# a release should run the equivalent steps manually:
#     echo X.Y.Z > VERSION
#     git commit -am "Release vX.Y.Z"
#     git tag -a vX.Y.Z -m "Release vX.Y.Z"
#     git push --follow-tags

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 0.1.0"
    exit 1
fi

VERSION="$1"
TAG="v$VERSION"

# Validate semver X.Y.Z. Pre-release suffixes (-rc1, -alpha, etc.) are not
# accepted here — keep them out of release tags until we actually need them.
if ! printf '%s' "$VERSION" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Error: version must be X.Y.Z (got: $VERSION)" >&2
    exit 1
fi

# Must be at repo root
if [ ! -f Makefile ] || [ ! -d include/jcccol ]; then
    echo "Error: run from the libjcccol repo root" >&2
    exit 1
fi

# Preflight: clean working tree
if ! git diff-index --quiet HEAD --; then
    echo "Error: working tree has uncommitted changes" >&2
    git status --short >&2
    exit 1
fi

# Preflight: on master or main
BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "$BRANCH" != "master" ] && [ "$BRANCH" != "main" ]; then
    echo "Error: release must be cut from master or main (currently on '$BRANCH')" >&2
    exit 1
fi

# Preflight: tag does not already exist locally or on origin
if git rev-parse --verify --quiet "refs/tags/$TAG" >/dev/null; then
    echo "Error: tag $TAG already exists locally" >&2
    exit 1
fi
remote_tag=$(git ls-remote --tags origin "refs/tags/$TAG" 2>/dev/null || true)
if [ -n "$remote_tag" ]; then
    echo "Error: tag $TAG already exists on origin" >&2
    exit 1
fi

# Preflight: tests pass. Shipping a broken release is the worst kind of bug.
echo "==> Running full build + tests"
make clean
make test

# Bump VERSION file (or skip if already at the target). The no-change
# case covers the very first release (VERSION shipped at 0.1.0 from the
# start) and any workflow where VERSION was hand-edited and committed
# beforehand. Without this check, `git commit` would fail on an empty
# diff and set -e would abort the whole release.
CURRENT_VERSION="$(cat VERSION 2>/dev/null || true)"
COMMITTED=0
if [ "$CURRENT_VERSION" = "$VERSION" ]; then
    echo "==> VERSION already $VERSION — skipping bump commit"
else
    echo "==> Writing VERSION=$VERSION"
    echo "$VERSION" > VERSION
    echo "==> Committing release bump"
    git add VERSION
    git commit -m "Release $TAG"
    COMMITTED=1
fi

# Annotated tag so it shows in `git describe` and carries a message.
echo "==> Creating annotated tag $TAG"
git tag -a "$TAG" -m "Release $TAG"

echo
echo "Local commit and tag created:"
git --no-pager log -1 --oneline
git --no-pager tag -n1 "$TAG"
echo
echo "To publish (this triggers the release workflow on GitHub):"
echo "    git push --follow-tags origin $BRANCH"
echo
echo "To abort:"
echo "    git tag -d $TAG"
if [ "$COMMITTED" = "1" ]; then
    echo "    git reset --hard HEAD~1"
fi
