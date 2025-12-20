# Repository Guidelines

## Project Structure & Module Organization

The project follows a standard directory structure:
- src/: Contains main source code files
- 	ests/: Unit tests and integration tests
- ssets/: Static resources like images, configs, etc.
- docs/: Documentation files
- scripts/: Build and utility scripts

Example: src/main.py contains the core application logic.

## Build, Test, and Development Commands

To develop locally:
- python setup.py develop - Installs package in development mode
- pytest - Runs all tests
- make build - Builds the application (if Makefile exists)
- lask run - Runs the web server (if applicable)

## Coding Style & Naming Conventions

- Use 4-space indentation
- Follow PEP8 for Python code
- Use snake_case for variables and functions
- Use PascalCase for classes
- All files should have a descriptive name starting with lowercase letter

## Testing Guidelines

Tests are written using pytest framework.
- Test files should be named 	est_*.py
- Each test should be isolated and self-contained
- Coverage target: 90% minimum
- Run tests with: pytest -v

## Commit & Pull Request Guidelines

Commit messages should follow conventional commits:
- eat:, ix:, docs:, style:, efactor:, perf:, 	est:, chore:
- Include issue number if fixing a bug: ix #123
- PRs must include:
  - Clear description of changes
  - Link to relevant issues
  - Screenshots or GIFs if visual changes
  - Test coverage report
