# Contributing to ESPHome Gree HVAC

Thank you for your interest in contributing to the ESPHome Gree HVAC component! This document provides information to help you get started.

## Development Environment

### GitHub Copilot AI Assistance

This project welcomes contributions from developers using AI-assisted coding tools. Below is information about GitHub Copilot's paid plans and available models for contributors who wish to use AI assistance.

#### GitHub Copilot Paid Plans and Available Models (2026)

GitHub Copilot offers several paid tiers with access to different AI models:

##### Individual Plans

**Copilot Pro** - $10/month or $100/year
- Unlimited code completions
- 300 premium requests/month (additional at $0.04/request)
- Unlimited chat and agent mode
- **Available Models:**
  - Anthropic Claude Sonnet 4.5
  - Google Gemini 2.5 Pro
  - OpenAI GPT-4.1
  - OpenAI GPT-5 mini
- Free for verified students, teachers, and open-source maintainers

**Copilot Pro+** - $39/month or $390/year
- All Pro features
- 1,500 premium requests/month (additional at $0.04/request)
- Priority access to experimental features
- **Available Models:**
  - All Pro models, plus:
  - Anthropic Claude Opus 4.1
  - Google Gemini 3 Pro (Preview)
  - OpenAI GPT-5.1
  - OpenAI Codex models

##### Organization Plans

**Copilot Business** - $19/user/month
- For teams and organizations
- 300 premium requests/user/month
- Centralized management and policy controls
- SAML SSO
- Code review and Copilot agent included
- **Available Models:** Same as Copilot Pro
- Additional features: Audit logs, business-specific controls

**Copilot Enterprise** - $39/user/month
- For large organizations
- 1,000 premium requests/user/month
- Custom models and deep knowledge base integration
- Full compliance and security features
- **Available Models:** All available models plus custom model options
- Additional features: Custom models, enhanced audit logs, advanced compliance

##### Free Tier

**Copilot Free** - $0
- 2,000 code completions/month
- 50 premium requests/month
- Limited chat requests
- **Available Models:**
  - Claude 4.5
  - GPT-4.1
  - Basic model access

#### Model Selection Tips for This Project

When working on this ESPHome component:

- **For C++ development** (gree_ac.cpp, gree_ac.h): Models with strong systems programming knowledge like Claude Sonnet or GPT-5 work well
- **For Python configuration** (climate.py, __init__.py): Most models handle Python effectively
- **For YAML configurations**: All models can assist with YAML editing
- **For protocol debugging**: Advanced models (Claude Opus, GPT-5.1) in Pro+ or Enterprise tiers may provide better assistance with binary protocol analysis

## Getting Started

### Prerequisites

- ESPHome 2024+ installed
- ESP32 or ESP8266 board
- Gree or compatible AC unit with UART interface
- Basic knowledge of UART protocols and electronics

### Setting Up Development Environment

1. Fork this repository
2. Clone your fork locally
3. Install ESPHome: `pip install esphome`
4. Create a test configuration based on `examples/example.yaml`

### Running Tests

Run the unit tests to verify protocol logic:

```bash
python3 tests/test_gree_ac_protocol.py
```

### Testing Changes

1. Modify the component code
2. Update your test YAML configuration
3. Compile: `esphome compile your-config.yaml`
4. Upload to your ESP: `esphome upload your-config.yaml`
5. Monitor logs: `esphome logs your-config.yaml`

### Code Style

- Follow existing code conventions in the repository
- C++ code should match ESPHome style guidelines
- Python code should follow PEP 8
- Add comments for complex protocol logic

## Submitting Changes

1. Create a new branch for your feature or fix
2. Make your changes with clear, descriptive commit messages
3. Test thoroughly with actual hardware if possible
4. Submit a pull request with:
   - Clear description of changes
   - Hardware tested on (if applicable)
   - Any relevant log outputs

## Reporting Issues

When reporting issues, please include:
- Your ESPHome version
- Your AC model and brand
- YAML configuration (sanitized)
- Relevant log output with `logger: level: VERBOSE`
- Packet traces if investigating protocol issues

## Areas for Contribution

We welcome contributions in these areas:

- **Protocol improvements**: Better error handling, edge cases
- **Feature implementation**: Plasma, Sleep, X-Fan modes
- **Hardware compatibility**: Testing and validation on different AC models
- **Documentation**: Installation guides, troubleshooting tips
- **Examples**: Additional configuration examples

## License

By contributing, you agree that your contributions will be licensed under the GPL-3.0 License.

## Questions?

Feel free to open an issue for questions or discussion about potential contributions.
