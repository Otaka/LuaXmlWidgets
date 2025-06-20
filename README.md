# LuaXmlWidgets

**Work in progress project, not working yet as expected**

A lightweight, cross-platform GUI framework that combines XML-based UI markup with Lua scripting to create native desktop applications using wxWidgets.

## Overview

LuaXmlWidgets allows developers to create desktop applications by writing XML-like markup files (`.lxml`) with embedded Lua scripts. 
The framework parses these files and generates native GUI components using the wxWidgets library, providing a declarative approach to desktop UI development.

## Features

- 🎯 **Declarative UI**: Define interfaces using intuitive XML markup
- ⚡ **Lua Scripting**: Embedded Lua scripts for dynamic behavior and event handling
- 🖥️ **Native Widgets**: Uses wxWidgets for truly native look and feel
- 🔄 **Cross-Platform**: Potentially runs on Windows, macOS, and Linux(right now only macOS)
- 📦 **Lightweight**: Minimal dependencies, small footprint
- 🏗️ **Embeddable**: Can be integrated into existing C++ applications
- 🎨 **Dynamic Updates**: Modify UI elements at runtime through Lua

## Quick Start

### Prerequisites

- C++11 compatible compiler
- wxWidgets 3.0+
- Lua 5.1+ (embedded via minilua)
- CMake or Xcode (for building)

### Building

```bash
# Clone the repository
git clone <repository-url>
cd LuaXmlWidgets

# Build with Xcode (macOS)
xcodebuild -project LuaXmlWidgets.xcodeproj -scheme LuaXmlWidgets
```

### Basic Usage

1. **Create an LXML file** (`hello.lxml`):
```xml
<App>
    <Script>
        function onButtonClick(event)
            print("Hello from Lua!")
            document:getElementById("label1"):setAttribute("text", "Button clicked!")
        end
    </Script>
    
    <Window text="Hello World" visible=true>
        <Label id="label1" x=10 y=10 text="Welcome to LuaXmlWidgets"/>
        <Button x=10 y=40 text="Click Me" onClick="onButtonClick"/>
    </Window>
</App>
```

2. **Run the application**:
```bash
./LuaXmlWidgets /path/to/your/files hello.lxml
```

## LXML Syntax

### Basic Structure

```xml
<App>
    <Script>
        -- Lua code here
        function eventHandler(event)
            -- Handle events
        end
    </Script>
    
    <Window text="My App" visible=true>
        <!-- UI components -->
    </Window>
</App>
```

### Supported Components

| Component | Description | Key Attributes |
|-----------|-------------|----------------|
| `Window` | Main application window | `text`, `visible`, `x`, `y`, `width`, `height` |
| `Button` | Clickable button | `text`, `onClick` |
| `Label` | Text display | `text`, `fgcolor`, `bgcolor`, `border` |
| `TextBox` | Text input field | `text`, `onChange` |
| `Tree` | Tree view control | `multipleSelection`, `rowLines` |
| `TreeNode` | Tree item | `text`, `bold`, `fgcolor`, `bgcolor` |
| `GlobalHotkey` | System-wide hotkey | `hotkey`, `onHotkey` |

### Common Attributes

- **Position**: `x`, `y`, `width`, `height`
- **Appearance**: `fgcolor`, `bgcolor`, `border`, `tooltip`
- **Behavior**: `visible`, `enabled`, `autoresize`
- **Events**: `onClick`, `onChange`, `onHotkey`

### Lua Integration

#### Accessing DOM Elements

```lua
-- Get element by ID
local button = document:getElementById("myButton")

-- Get/set attributes
local text = button:getAttribute("text")
button:setAttribute("text", "New Text")

-- Dynamic content
button:setAttribute("innerLXML", "<Label text='Dynamic content'/>")
```

#### Event Handling

```lua
function onButtonClick(event)
    local sender = event.sender  -- The element that triggered the event
    print("Button clicked:", sender:getAttribute("text"))
end
```

#### Built-in Functions

```lua
-- Message boxes
MessageBox("Message", "Title", {"OK", "Cancel"}, "info")

-- Console output
print("Debug message")

-- Element manipulation
element:remove()  -- Remove from DOM
```

## Project Structure

```
LuaXmlWidgets/
├── src/
│   ├── lxe*.hpp/cpp     # Core engine (parsing, DOM, Lua integration)
│   ├── lxw*.hpp/cpp     # Widget implementations
│   ├── main.cpp         # Application entry point
│   └── tests/           # Unit tests
├── examples/
│   └── main.lxml        # Example application
├── lua_std_lib/         # Embedded Lua standard library
└── tools/               # Build utilities
```

### Core Components

- **lxeEngine**: DOM element management and lifecycle
- **lxeParser**: LXML file parsing and tokenization
- **lxeLua**: Lua script engine integration
- **lxwControls**: wxWidgets component implementations
- **lxwGui**: Main application framework

## Architecture

```
┌─────────────────┐    ┌─────────────────┐
│   LXML Files    │───▶│   LXE Parser    │
└─────────────────┘    └─────────────────┘
                                │
                                ▼
┌─────────────────┐    ┌─────────────────┐
│  Lua Scripts    │◀──▶│  DOM Engine     │
└─────────────────┘    └─────────────────┘
                                │
                                ▼
┌─────────────────┐    ┌─────────────────┐
│   User Input    │◀──▶│  wxWidgets UI   │
└─────────────────┘    └─────────────────┘
```

## Advanced Features

### Dynamic UI Updates

```lua
-- Replace content dynamically
newContent = [[
    <Button text="Dynamic Button" onClick="dynamicHandler"/>
    <Label text="Added at runtime"/>
]]
container:setAttribute("innerLXML", newContent)
```

### Resource Management

```lua
-- Load resources
require "resource://lxw/lxw.lua"
```

### Event System

The framework provides a comprehensive event system:

- **Mouse Events**: `onClick`, `onDoubleClick`
- **Keyboard Events**: Global hotkeys, key press handlers
- **Change Events**: `onChange` for input fields
- **Custom Events**: Define your own event types

## Examples

### Simple Calculator

```xml
<App>
    <Script>
        function calculate()
            local a = tonumber(document:getElementById("input1"):getAttribute("text"))
            local b = tonumber(document:getElementById("input2"):getAttribute("text"))
            document:getElementById("result"):setAttribute("text", tostring(a + b))
        end
    </Script>
    
    <Window text="Calculator" visible=true>
        <TextBox id="input1" x=10 y=10 width=100/>
        <Label x=120 y=10 text="+"/>
        <TextBox id="input2" x=140 y=10 width=100/>
        <Button x=250 y=10 text="=" onClick="calculate"/>
        <Label id="result" x=10 y=40 text="Result"/>
    </Window>
</App>
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Development Setup

### Building from Source

1. Install dependencies:
   - wxWidgets development libraries
   - C++11 compatible compiler

2. Configure build:
```bash
# For Xcode
open LuaXmlWidgets.xcodeproj

# For command line
xcodebuild -list  # See available schemes
```

3. Run tests:
```bash
xcodebuild -project LuaXmlWidgets.xcodeproj -scheme Test
```
