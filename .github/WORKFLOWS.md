# GitHub Actions Workflows

This repository includes comprehensive CI/CD automation:

## Workflows

### 1. CI (`ci.yml`)
**Triggers**: Every push and PR to main
**Purpose**: Quick validation and feedback
- Debug build and test
- Static analysis (Clang-Tidy, non-blocking)
- Release build compilation check
- Code formatting validation

### 2. Quality Assurance (`quality-assurance.yml`)
**Triggers**: Push/PR to main and develop
**Purpose**: Comprehensive quality validation
- Static analysis (Clang-Tidy)
- Sanitizer testing (ASAN, TSAN, UBSAN)
- Cross-platform builds (Ubuntu, macOS, Windows)
- Full QA suite execution

### 3. Build and Test (`build-test.yml`)
**Triggers**: Push/PR to main and develop  
**Purpose**: Multi-platform, multi-compiler validation
- Ubuntu + macOS
- GCC + Clang compilers
- Debug + Release builds
- Comprehensive test execution

### 4. Release (`release.yml`)
**Triggers**: Version tags (`v*`) and manual dispatch
**Purpose**: Automated release process
- Full QA validation
- Multi-platform release builds
- Package generation
- GitHub release creation

## CMake Presets for CI

- `ci`: Optimized for continuous integration with warnings as errors
- `dev-debug`: Development debug builds
- `dev-release`: Development release builds  
- `clang-tidy`: Static analysis builds
- `asan`, `tsan`, `ubsan`: Sanitizer builds

## Quality Metrics

All workflows validate:
- ✅ Memory safety (AddressSanitizer)
- ✅ Thread safety (ThreadSanitizer) 
- ✅ Undefined behavior (UBSanitizer)
- ✅ Static analysis (Clang-Tidy, 100+ checks)
- ✅ Cross-platform compilation
- ✅ Multi-compiler support
- ✅ Zero-overhead release builds