# Resolve the git hash of the sources the build is starting from, print it to
# the build log and write it into a generated header.
#
# Run with `cmake -P` from a custom target on EVERY build. The value cannot be
# resolved at configure time: committing does not re-run cmake, so a configure
# time hash keeps naming the commit that was checked out when cmake last ran --
# a stale hash is worse than none, because it claims a provenance the binary
# does not have. Resolving it here has the second benefit of showing it in the
# build output, which is where a developer looks when a build is questionable.
#
# Everything this needs is resolved here, git included: the only thing the
# calling CMakeLists.txt has to know is where the checkout and the output header
# are, so it passes no tool paths around.
#
# Inputs (-D):
#   REPO_DIR           the source checkout (the directory owning .git)
#   OUTPUT_FILE        header to write; rewritten only when the hash changes so
#                      an unchanged build does not recompile or relink
#   GIT_HASH_OVERRIDE  explicit hash for a build without git metadata
#
# Consumed by src/Launchy/LaunchyVersion.h as LAUNCHY_GIT_HASH.

# Script mode has no cache, so this searches PATH (and the usual Windows
# locations) on every build. When git is missing find_package leaves
# GIT_EXECUTABLE at a "-NOTFOUND" value, which if() already treats as false.
find_package (Git QUIET)

set (hash "")
set (state "")

if (GIT_HASH_OVERRIDE)
  set (hash "${GIT_HASH_OVERRIDE}")
  set (state "from GIT_HASH_OVERRIDE")

elseif (GIT_EXECUTABLE AND EXISTS "${REPO_DIR}/.git")
  execute_process (
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short=8 HEAD
    WORKING_DIRECTORY "${REPO_DIR}"
    OUTPUT_VARIABLE hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE gitResult)

  if (NOT gitResult EQUAL 0)
    message (STATUS "Launchy build info: git rev-parse failed in ${REPO_DIR}")
    set (hash "")
  else ()
    # Uncommitted changes mean the tree no longer matches the commit the hash
    # names, so say so instead of pretending. Untracked files are ignored: a
    # build directory, a scratch file, an editor backup -- none of them end up
    # in the binary.
    execute_process (
      COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=no
      WORKING_DIRECTORY "${REPO_DIR}"
      OUTPUT_VARIABLE gitStatus
      ERROR_QUIET)

    if (gitStatus STREQUAL "")
      set (state "clean")
    else ()
      string (REGEX REPLACE "\n$" "" gitStatus "${gitStatus}")
      string (REPLACE "\n" ";" modifiedFiles "${gitStatus}")
      list (LENGTH modifiedFiles modifiedCount)
      set (hash "${hash}-dirty")
      set (state "dirty, ${modifiedCount} modified file(s)")
    endif ()
  endif ()

elseif (NOT GIT_EXECUTABLE)
  message (STATUS "Launchy build info: git not found, cannot resolve the hash")

else ()
  message (STATUS "Launchy build info: ${REPO_DIR} is not a git checkout")
endif ()

if (NOT hash)
  set (hash "unknown")
  if (NOT state)
    set (state "no git metadata")
  endif ()
endif ()

set (content "// Generated during every build by src/cmake/GenerateBuildInfo.cmake -- do not edit.
//
// Commit the sources were at when this build started, with a \"-dirty\" suffix
// when the working tree had uncommitted changes. Included by LaunchyVersion.h
// and shown on the About page.
#pragma once

#define LAUNCHY_GIT_HASH \"${hash}\"
")

set (oldContent "")
if (EXISTS "${OUTPUT_FILE}")
  file (READ "${OUTPUT_FILE}" oldContent)
endif ()

if (NOT oldContent STREQUAL content)
  get_filename_component (outputDir "${OUTPUT_FILE}" DIRECTORY)
  file (MAKE_DIRECTORY "${outputDir}")
  file (WRITE "${OUTPUT_FILE}" "${content}")
  message (STATUS "Launchy build info: git hash ${hash} (${state}) -> header updated")
else ()
  message (STATUS "Launchy build info: git hash ${hash} (${state})")
endif ()
