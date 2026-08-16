# GitHub Actions Cloud Build Handoff

## Verified Access

- GitHub account: clxiny
- CLI: GitHub CLI 2.97.0, authenticated through device login
- Repository: clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput
- Workflow: .github/workflows/custom-windows-x64.yml
- Latest successful test: run 31936157173, completed in 3m28s on 2026-08-16
- Artifact: QtScrcpy-custom-v4.1-input-recovery-win-x64
- Artifact digest: sha256:bff8db8feac849cf0c69bc2467c344f13d5ee69a29c6e56cca870e706334e7d5

## Useful Links

- Actions run: https://github.com/clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput/actions/runs/31936157173
- Actions page: https://github.com/clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput/actions
- Workflow source: https://github.com/clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput/blob/main/.github/workflows/custom-windows-x64.yml

## Commands

Run these from the project repository after installing/authenticating GitHub CLI:

    gh workflow run custom-windows-x64.yml --repo clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput --ref main
    gh run list --repo clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput --workflow custom-windows-x64.yml --limit 5
    gh run watch <run-id> --repo clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput --exit-status
    gh run download <run-id> --repo clxiny/QtScrcpy-custom-build-v4.1-fps-250hz-rawinput --dir artifacts

## New-Chat Prompt

    Please use GitHub Actions to build this project in the cloud. GitHub CLI is installed locally and authenticated as clxiny; first run gh auth status to confirm it. The repository is <owner/repo>, and the branch is <branch>. First inspect existing .github/workflows and the build entry points, then create or update the workflow for the required target platforms and architectures. Trigger the build, read logs until completion, and download/validate the artifact. Do not require a local cross-compilation environment; prefer GitHub-hosted runners. Report the Actions run URL, artifact URL/name, SHA-256, failed step if any, and the smallest useful fix.

For a mixed-language project, include supported OS/architectures, build systems (CMake/Gradle/npm/etc.), compiler/SDK versions, signing or secret requirements, and expected artifact names.
