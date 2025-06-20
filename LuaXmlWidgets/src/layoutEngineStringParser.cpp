//
//  layoutEngineStringParser.cpp
//  LuaXmlWidgets
//

#include "layoutEngine.hpp"
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <set>

namespace LayoutEngine {

//==============================================================================
// String Parsing Utilities
//==============================================================================

/**
 * Splits a string by delimiter
 */
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        // Trim whitespace
        token.erase(0, token.find_first_not_of(" \t\n\r"));
        token.erase(token.find_last_not_of(" \t\n\r") + 1);
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

/**
 * Converts string to lowercase
 */
std::string toLowerCase(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

/**
 * Parses a float value from string, throws exception if invalid
 */
float parseFloat(const std::string& str) {
    if (str.empty()) {
        throw ConstraintParseException("Empty value where number expected");
    }
    
    try {
        size_t pos;
        float result = std::stof(str, &pos);
        
        // Check if the entire string was consumed (allowing for unit suffixes)
        std::string remaining = str.substr(pos);
        if (!remaining.empty() && remaining != "px" && remaining != "pt" && remaining != "%" && remaining != "!") {
            throw ConstraintParseException("Invalid number format: '" + str + "'");
        }
        
        return result;
    } catch (const std::invalid_argument&) {
        throw ConstraintParseException("Invalid number format: '" + str + "'");
    } catch (const std::out_of_range&) {
        throw ConstraintParseException("Number out of range: '" + str + "'");
    }
}

/**
 * Parses an integer value from string, throws exception if invalid
 */
int parseInt(const std::string& str) {
    if (str.empty()) {
        throw ConstraintParseException("Empty value where integer expected");
    }
    
    try {
        size_t pos;
        int result = std::stoi(str, &pos);
        
        // Check if the entire string was consumed
        if (pos != str.length()) {
            throw ConstraintParseException("Invalid integer format: '" + str + "'");
        }
        
        return result;
    } catch (const std::invalid_argument&) {
        throw ConstraintParseException("Invalid integer format: '" + str + "'");
    } catch (const std::out_of_range&) {
        throw ConstraintParseException("Integer out of range: '" + str + "'");
    }
}

/**
 * Safely parses integer with optional default for missing values
 */
int parseIntWithDefault(const std::vector<std::string>& parts, size_t index, int defaultValue) {
    if (index >= parts.size()) {
        return defaultValue;
    }
    return parseInt(parts[index]);
}

/**
 * Safely parses float with optional default for missing values
 */
float parseFloatWithDefault(const std::vector<std::string>& parts, size_t index, float defaultValue) {
    if (index >= parts.size()) {
        return defaultValue;
    }
    return parseFloat(parts[index]);
}

/**
 * Validates that required parameter exists
 */
void requireParameter(const std::vector<std::string>& parts, size_t index, const std::string& command) {
    if (index >= parts.size()) {
        throw ConstraintParseException("Missing required parameter for '" + command + "'");
    }
}

/**
 * Parses a size constraint from string (e.g., "100px", "50%", "min:pref:max", "pref")
 */
SizeConstraint parseSizeConstraint(const std::string& str) {
    if (str.empty() || str == "pref") {
        return SizeConstraint::content();
    }
    
    std::string s = toLowerCase(str);
    
    // Check for percentage
    if (s.find('%') != std::string::npos) {
        float percent = parseFloat(s.substr(0, s.find('%')));
        return SizeConstraint::percentage(percent);
    }
    
    // Check for range format (min:pref:max)
    if (s.find(':') != std::string::npos) {
        auto parts = split(s, ':');
        if (parts.size() >= 3) {
            float min = parseFloat(parts[0]);
            float pref = parseFloat(parts[1]);
            float max = parseFloat(parts[2]);
            return SizeConstraint::range(min, pref, max);
        } else if (parts.size() == 2) {
            float min = parseFloat(parts[0]);
            float pref = parseFloat(parts[1]);
            return SizeConstraint::range(min, pref, std::numeric_limits<float>::max());
        }
    }
    
    // Remove unit suffixes (px, pt, etc.) for now - just parse the number
    std::string numStr = s;
    if (s.find("px") != std::string::npos) {
        numStr = s.substr(0, s.find("px"));
    } else if (s.find("pt") != std::string::npos) {
        numStr = s.substr(0, s.find("pt"));
    }
    
    // Check for fixed size with exclamation mark
    bool isFixed = false;
    if (numStr.find('!') != std::string::npos) {
        isFixed = true;
        numStr.erase(std::remove(numStr.begin(), numStr.end(), '!'), numStr.end());
    }
    
    float value = parseFloat(numStr);
    if (isFixed) {
        return SizeConstraint::fixed(value);
    }
    
    return SizeConstraint::range(0, value, std::numeric_limits<float>::max());
}

/**
 * Parses alignment from string
 */
Alignment parseAlignment(const std::string& str) {
    std::string s = toLowerCase(str);
    
    if (s == "start" || s == "left" || s == "top" || s == "leading") {
        return Alignment::Start;
    } else if (s == "center" || s == "centre") {
        return Alignment::Center;
    } else if (s == "end" || s == "right" || s == "bottom" || s == "trailing") {
        return Alignment::End;
    } else if (s == "fill" || s == "stretch") {
        return Alignment::Fill;
    } else if (s == "baseline") {
        return Alignment::Baseline;
    }
    
    return Alignment::Center; // Default
}

/**
 * Parses border side from string
 */
BorderSide parseBorderSide(const std::string& str) {
    std::string s = toLowerCase(str);
    
    if (s == "north" || s == "top") {
        return BorderSide::Top;
    } else if (s == "south" || s == "bottom") {
        return BorderSide::Bottom;
    } else if (s == "west" || s == "left") {
        return BorderSide::Left;
    } else if (s == "east" || s == "right") {
        return BorderSide::Right;
    }
    
    return BorderSide::None;
}

/**
 * Parses hide mode from string
 */
HideMode parseHideMode(const std::string& str) {
    int mode = parseInt(str);
    switch (mode) {
        case 0: return HideMode::Default;
        case 3: return HideMode::Exclude;
        default: 
            throw ConstraintParseException("Invalid hide mode: '" + str + "'. Valid values are 0 or 3");
    }
}

//==============================================================================
// Layout Constraint Parser
//==============================================================================

/**
 * Parses layout constraint string and returns a configured FlexGridLayout
 * Example: "wrap 3, gap 10px 5px, insets 20, fill, debug"
 */
FlexGridLayout* parseLayoutConstraints(const std::string& constraintStr) {
    FlexGridLayout* layout = new FlexGridLayout();
    
    if (constraintStr.empty()) {
        return layout;
    }
    
    // Valid layout constraint keywords
    static const std::set<std::string> validLayoutKeywords = {
        "wrap", "gap", "gapx", "gapy", "insets", "ins", "fill", "fillx", "filly",
        "flowy", "nogrid", "debug", "hidemode", "align", "al", "alignx", "ax", "aligny", "ay"
    };
    
    // Split by commas
    auto tokens = split(constraintStr, ',');
    
    for (const auto& token : tokens) {
        auto parts = split(token, ' ');
        if (parts.empty()) continue;
        
        std::string command = toLowerCase(parts[0]);
        
        // Validate command
        if (validLayoutKeywords.find(command) == validLayoutKeywords.end()) {
            throw ConstraintParseException("Unknown layout constraint: '" + command + "'");
        }
        
        if (command == "wrap") {
            int columns = parseIntWithDefault(parts, 1, -1);
            layout->setWrap(columns);
        }
        else if (command == "gap") {
            if (parts.size() >= 3) {
                float hgap = parseFloat(parts[1]);
                float vgap = parseFloat(parts[2]);
                layout->setGap(hgap, vgap);
            } else if (parts.size() >= 2) {
                float gap = parseFloat(parts[1]);
                layout->setGap(gap, gap);
            } else {
                throw ConstraintParseException("'gap' requires at least one parameter");
            }
        }
        else if (command == "gapx") {
            requireParameter(parts, 1, "gapx");
            float gap = parseFloat(parts[1]);
            layout->setGap(gap, 5.0f); // Default vertical gap
        }
        else if (command == "gapy") {
            requireParameter(parts, 1, "gapy");
            float gap = parseFloat(parts[1]);
            layout->setGap(5.0f, gap); // Default horizontal gap
        }
        else if (command == "insets" || command == "ins") {
            if (parts.size() >= 5) {
                float top = parseFloat(parts[1]);
                float left = parseFloat(parts[2]);
                float bottom = parseFloat(parts[3]);
                float right = parseFloat(parts[4]);
                layout->setInsets(top, left, bottom, right);
            } else if (parts.size() >= 2) {
                float inset = parseFloat(parts[1]);
                layout->setInsets(inset, inset, inset, inset);
            }
        }
        else if (command == "fill") {
            layout->setFill(true, true);
        }
        else if (command == "fillx") {
            layout->setFill(true, false);
        }
        else if (command == "filly") {
            layout->setFill(false, true);
        }
        else if (command == "flowy") {
            layout->setFlowDirection(FlowDirection::Vertical);
        }
        else if (command == "nogrid") {
            layout->setNoGrid(true);
        }
        else if (command == "debug") {
            layout->setDebugMode(true);
        }
        else if (command == "hidemode" && parts.size() >= 2) {
            HideMode mode = parseHideMode(parts[1]);
            layout->setHideMode(mode);
        }
        else if (command == "align" || command == "al") {
            if (parts.size() >= 3) {
                Alignment hAlign = parseAlignment(parts[1]);
                Alignment vAlign = parseAlignment(parts[2]);
                layout->setAlignment(hAlign, vAlign);
            } else if (parts.size() >= 2) {
                Alignment align = parseAlignment(parts[1]);
                layout->setAlignment(align, align);
            }
        }
        else if (command == "alignx" || command == "ax") {
            if (parts.size() >= 2) {
                Alignment align = parseAlignment(parts[1]);
                layout->setAlignment(align, Alignment::Fill);
            }
        }
        else if (command == "aligny" || command == "ay") {
            if (parts.size() >= 2) {
                Alignment align = parseAlignment(parts[1]);
                layout->setAlignment(Alignment::Fill, align);
            }
        }
    }
    
    return layout;
}

//==============================================================================
// Entity Constraint Parser
//==============================================================================

/**
 * Parses entity constraint string and returns a configured EntityConstraints
 * Example: "width 100px!, grow, span 2, alignx fill, wrap"
 */
EntityConstraints* parseEntityConstraints(const std::string& constraintStr) {
    EntityConstraints* constraints = new EntityConstraints();
    
    if (constraintStr.empty()) {
        return constraints;
    }
    
    // Valid entity constraint keywords
    static const std::set<std::string> validEntityKeywords = {
        "width", "w", "height", "h", "wmin", "wmax", "hmin", "hmax",
        "align", "al", "alignx", "ax", "aligny", "ay", 
        "span", "spanx", "sx", "spany", "sy",
        "grow", "growx", "growy", "push", "pushx", "pushy", "shrink",
        "wrap", "newline", "skip", "split", "flowx", "flowy",
        "cell", "sizegroup", "sg", "endgroup", "eg", "id",
        "dock", "north", "south", "east", "west", "hidemode",
        "left", "right", "top", "bottom", "center", "fill"
    };
    
    // Split by commas
    auto tokens = split(constraintStr, ',');
    
    for (const auto& token : tokens) {
        auto parts = split(token, ' ');
        if (parts.empty()) continue;
        
        std::string command = toLowerCase(parts[0]);
        
        // Validate command
        if (validEntityKeywords.find(command) == validEntityKeywords.end()) {
            throw ConstraintParseException("Unknown entity constraint: '" + command + "'");
        }
        
        // Size constraints
        if ((command == "width" || command == "w") && parts.size() >= 2) {
            SizeConstraint width = parseSizeConstraint(parts[1]);
            constraints->setWidth(width);
        }
        else if ((command == "height" || command == "h") && parts.size() >= 2) {
            SizeConstraint height = parseSizeConstraint(parts[1]);
            constraints->setHeight(height);
        }
        else if (command == "wmin" && parts.size() >= 2) {
            SizeConstraint minWidth = parseSizeConstraint(parts[1]);
            constraints->setMinWidth(minWidth);
        }
        else if (command == "wmax" && parts.size() >= 2) {
            SizeConstraint maxWidth = parseSizeConstraint(parts[1]);
            constraints->setMaxWidth(maxWidth);
        }
        else if (command == "hmin" && parts.size() >= 2) {
            SizeConstraint minHeight = parseSizeConstraint(parts[1]);
            constraints->setMinHeight(minHeight);
        }
        else if (command == "hmax" && parts.size() >= 2) {
            SizeConstraint maxHeight = parseSizeConstraint(parts[1]);
            constraints->setMaxHeight(maxHeight);
        }
        
        // Alignment
        else if (command == "align" || command == "al") {
            if (parts.size() >= 3) {
                Alignment hAlign = parseAlignment(parts[1]);
                Alignment vAlign = parseAlignment(parts[2]);
                constraints->setHorizontalAlign(hAlign)->setVerticalAlign(vAlign);
            } else if (parts.size() >= 2) {
                Alignment align = parseAlignment(parts[1]);
                constraints->setHorizontalAlign(align)->setVerticalAlign(align);
            }
        }
        else if (command == "alignx" || command == "ax") {
            if (parts.size() >= 2) {
                Alignment align = parseAlignment(parts[1]);
                constraints->setHorizontalAlign(align);
            }
        }
        else if (command == "aligny" || command == "ay") {
            if (parts.size() >= 2) {
                Alignment align = parseAlignment(parts[1]);
                constraints->setVerticalAlign(align);
            }
        }
        
        // Spanning
        else if (command == "span") {
            if (parts.size() >= 3) {
                int spanX = parseInt(parts[1]);
                int spanY = parseInt(parts[2]);
                constraints->setSpanX(spanX)->setSpanY(spanY);
            } else if (parts.size() >= 2) {
                int span = parseInt(parts[1]);
                constraints->setSpanX(span);
            } else {
                constraints->setSpanX(999); // Span to end
            }
        }
        else if (command == "spanx" || command == "sx") {
            int span = parseIntWithDefault(parts, 1, 999);
            constraints->setSpanX(span);
        }
        else if (command == "spany" || command == "sy") {
            int span = parseIntWithDefault(parts, 1, 999);
            constraints->setSpanY(span);
        }
        
        // Growing and shrinking
        else if (command == "grow") {
            if (parts.size() >= 3) {
                float growX = parseFloat(parts[1]);
                float growY = parseFloat(parts[2]);
                constraints->setGrowX(growX)->setGrowY(growY);
            } else if (parts.size() >= 2) {
                float grow = parseFloat(parts[1]);
                constraints->setGrowX(grow)->setGrowY(grow);
            } else {
                constraints->setGrowX(100.0f)->setGrowY(100.0f);
            }
        }
        else if (command == "growx") {
            float grow = parseFloatWithDefault(parts, 1, 100.0f);
            constraints->setGrowX(grow);
        }
        else if (command == "growy") {
            float grow = parseFloatWithDefault(parts, 1, 100.0f);
            constraints->setGrowY(grow);
        }
        else if (command == "push") {
            if (parts.size() >= 3) {
                float pushX = parseFloat(parts[1]);
                float pushY = parseFloat(parts[2]);
                constraints->setGrowX(pushX)->setGrowY(pushY);
            } else if (parts.size() >= 2) {
                float push = parseFloat(parts[1]);
                constraints->setGrowX(push);
            } else {
                constraints->setGrowX(100.0f);
            }
        }
        else if (command == "pushx") {
            float push = parseFloatWithDefault(parts, 1, 100.0f);
            constraints->setGrowX(push);
        }
        else if (command == "pushy") {
            float push = parseFloatWithDefault(parts, 1, 100.0f);
            constraints->setGrowY(push);
        }
        else if (command == "shrink") {
            if (parts.size() >= 3) {
                float shrinkX = parseFloat(parts[1]);
                float shrinkY = parseFloat(parts[2]);
                constraints->setShrinkX(shrinkX)->setShrinkY(shrinkY);
            } else if (parts.size() >= 2) {
                float shrink = parseFloat(parts[1]);
                constraints->setShrinkX(shrink)->setShrinkY(shrink);
            } else {
                throw ConstraintParseException("'shrink' requires at least one parameter");
            }
        }
        
        // Flow control
        else if (command == "wrap") {
            constraints->setWrap(true);
        }
        else if (command == "newline") {
            constraints->setNewline(true);
        }
        else if (command == "skip") {
            int skip = parseIntWithDefault(parts, 1, 1);
            constraints->setSkip(skip);
        }
        else if (command == "split") {
            int split = parseIntWithDefault(parts, 1, 2);
            constraints->setSplit(split);
        }
        else if (command == "flowx") {
            constraints->setCellFlow(FlowDirection::Horizontal);
        }
        else if (command == "flowy") {
            constraints->setCellFlow(FlowDirection::Vertical);
        }
        
        // Grid positioning
        else if (command == "cell") {
            if (parts.size() < 3) {
                throw ConstraintParseException("'cell' requires at least 2 parameters (x, y)");
            }
            int x = parseInt(parts[1]);
            int y = parseInt(parts[2]);
            constraints->setCellX(x)->setCellY(y);
            if (parts.size() >= 5) {
                int spanX = parseInt(parts[3]);
                int spanY = parseInt(parts[4]);
                constraints->setSpanX(spanX)->setSpanY(spanY);
            }
        }
        
        // Grouping
        else if (command == "sizegroup" || command == "sg") {
            std::string group = parts.size() >= 2 ? parts[1] : "";
            constraints->setSizeGroup(group);
        }
        else if (command == "endgroup" || command == "eg") {
            std::string group = parts.size() >= 2 ? parts[1] : "";
            constraints->setEndGroup(group);
        }
        else if (command == "id") {
            std::string id = parts.size() >= 2 ? parts[1] : "";
            constraints->setComponentId(id);
        }
        
        // Docking
        else if (command == "dock") {
            if (parts.size() >= 2) {
                BorderSide side = parseBorderSide(parts[1]);
                constraints->setBorderAttachment(side);
            }
        }
        else if (command == "north" || command == "south" || command == "east" || command == "west") {
            BorderSide side = parseBorderSide(command);
            constraints->setBorderAttachment(side);
        }
        
        // Hide mode
        else if (command == "hidemode" && parts.size() >= 2) {
            HideMode mode = parseHideMode(parts[1]);
            constraints->setHideMode(mode);
        }
        
        // Simple alignment shortcuts
        else if (command == "left") {
            constraints->setHorizontalAlign(Alignment::Start);
        }
        else if (command == "right") {
            constraints->setHorizontalAlign(Alignment::End);
        }
        else if (command == "top") {
            constraints->setVerticalAlign(Alignment::Start);
        }
        else if (command == "bottom") {
            constraints->setVerticalAlign(Alignment::End);
        }
        else if (command == "center") {
            constraints->setHorizontalAlign(Alignment::Center)->setVerticalAlign(Alignment::Center);
        }
        else if (command == "fill") {
            constraints->setHorizontalAlign(Alignment::Fill)->setVerticalAlign(Alignment::Fill);
        }
        else {
            throw ConstraintParseException(std::string("Cannot parse the layout entity constraint: ")+constraintStr);
        }
    }
    
    return constraints;
}

//==============================================================================
// Phase 2: Enhanced GUI Integration Functions with Caching
//==============================================================================

// Cache for parsed constraints to avoid reparsing same strings
static std::unordered_map<std::string, EntityConstraints> constraintCache;
static std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> containerConfigCache;

/**
 * Parse container configuration string into key-value pairs
 */
std::vector<std::pair<std::string, std::string>> parseContainerConfigurationString(const std::string& config) {
    std::vector<std::pair<std::string, std::string>> result;
    
    if (config.empty()) return result;
    
    std::vector<std::string> tokens = split(config, ',');
    
    for (const std::string& token : tokens) {
        std::string trimmed = token;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) continue;
        
        std::vector<std::string> parts = split(trimmed, ' ');
        if (parts.empty()) continue;
        
        std::string key = toLowerCase(parts[0]);
        std::string value = "";
        
        if (parts.size() > 1) {
            // Reconstruct value from remaining parts
            for (size_t i = 1; i < parts.size(); ++i) {
                if (i > 1) value += " ";
                value += parts[i];
            }
        }
        
        result.push_back(std::make_pair(key, value));
    }
    
    return result;
}

/**
 * Apply parsed container configuration to layout manager
 */
void applyContainerConfiguration(FlexGridLayout* layoutManager, 
                                const std::vector<std::pair<std::string, std::string>>& config) {
    if (!layoutManager) return;
    
    for (const auto& option : config) {
        const std::string& key = option.first;
        const std::string& value = option.second;
        
        if (key == "gap") {
            float gap = 5.0f; // default
            if (!value.empty()) {
                try {
                    gap = parseFloat(value);
                } catch (...) {
                    gap = 5.0f;
                }
            }
            layoutManager->setGap(gap, gap);
        }
        else if (key == "gapx") {
            float gapX = 5.0f;
            if (!value.empty()) {
                try {
                    gapX = parseFloat(value);
                } catch (...) {
                    gapX = 5.0f;
                }
            }
            // Get current vertical gap and set horizontal gap
            float currentGapY = 5.0f; // TODO: Get from layout manager
            layoutManager->setGap(gapX, currentGapY);
        }
        else if (key == "gapy") {
            float gapY = 5.0f;
            if (!value.empty()) {
                try {
                    gapY = parseFloat(value);
                } catch (...) {
                    gapY = 5.0f;
                }
            }
            // Get current horizontal gap and set vertical gap
            float currentGapX = 5.0f; // TODO: Get from layout manager
            layoutManager->setGap(currentGapX, gapY);
        }
        else if (key == "fill") {
            bool horizontal = true;
            bool vertical = true;
            
            if (!value.empty()) {
                std::string fillValue = toLowerCase(value);
                if (fillValue == "x" || fillValue == "horizontal") {
                    horizontal = true;
                    vertical = false;
                } else if (fillValue == "y" || fillValue == "vertical") {
                    horizontal = false;
                    vertical = true;
                } else if (fillValue == "both" || fillValue == "true") {
                    horizontal = true;
                    vertical = true;
                } else if (fillValue == "none" || fillValue == "false") {
                    horizontal = false;
                    vertical = false;
                }
            }
            
            layoutManager->setFill(horizontal, vertical);
        }
        else if (key == "wrap") {
            int columns = -1; // auto-wrap
            if (!value.empty()) {
                try {
                    columns = static_cast<int>(parseFloat(value));
                } catch (...) {
                    columns = -1;
                }
            }
            layoutManager->setWrap(columns);
        }
        else if (key == "debug") {
            bool enable = true;
            if (!value.empty()) {
                std::string debugValue = toLowerCase(value);
                enable = (debugValue != "false" && debugValue != "0");
            }
            layoutManager->setDebugMode(enable);
        }
        else if (key == "nogrid") {
            bool enable = true;
            if (!value.empty()) {
                std::string nogridValue = toLowerCase(value);
                enable = (nogridValue != "false" && nogridValue != "0");
            }
            layoutManager->setNoGrid(enable);
        }
        else if (key == "insets") {
            if (!value.empty()) {
                std::vector<std::string> insetParts = split(value, ' ');
                if (insetParts.size() == 1) {
                    // All sides same
                    try {
                        float inset = parseFloat(insetParts[0]);
                        layoutManager->setInsets(Insets(inset));
                    } catch (...) {
                        // Ignore invalid inset values
                    }
                } else if (insetParts.size() == 4) {
                    // top, left, bottom, right
                    try {
                        float top = parseFloat(insetParts[0]);
                        float left = parseFloat(insetParts[1]);
                        float bottom = parseFloat(insetParts[2]);
                        float right = parseFloat(insetParts[3]);
                        layoutManager->setInsets(Insets(top, left, bottom, right));
                    } catch (...) {
                        // Ignore invalid inset values
                    }
                }
            }
        }
        else if (key == "flowdir" || key == "flow") {
            if (!value.empty()) {
                std::string flowValue = toLowerCase(value);
                if (flowValue == "horizontal" || flowValue == "h") {
                    layoutManager->setFlowDirection(FlowDirection::Horizontal);
                } else if (flowValue == "vertical" || flowValue == "v") {
                    layoutManager->setFlowDirection(FlowDirection::Vertical);
                }
            }
        }
        else if (key == "align") {
            if (!value.empty()) {
                std::vector<std::string> alignParts = split(value, ' ');
                if (alignParts.size() >= 1) {
                    std::string hAlign = toLowerCase(alignParts[0]);
                    Alignment horizontal = Alignment::Fill;
                    if (hAlign == "left" || hAlign == "start") {
                        horizontal = Alignment::Start;
                    } else if (hAlign == "center") {
                        horizontal = Alignment::Center;
                    } else if (hAlign == "right" || hAlign == "end") {
                        horizontal = Alignment::End;
                    } else if (hAlign == "fill") {
                        horizontal = Alignment::Fill;
                    }
                    
                    Alignment vertical = Alignment::Fill;
                    if (alignParts.size() >= 2) {
                        std::string vAlign = toLowerCase(alignParts[1]);
                        if (vAlign == "top" || vAlign == "start") {
                            vertical = Alignment::Start;
                        } else if (vAlign == "center") {
                            vertical = Alignment::Center;
                        } else if (vAlign == "bottom" || vAlign == "end") {
                            vertical = Alignment::End;
                        } else if (vAlign == "fill") {
                            vertical = Alignment::Fill;
                        }
                    }
                    
                    layoutManager->setAlignment(horizontal, vertical);
                }
            }
        }
    }
}

/**
 * Parse container layout configuration with caching
 */
void parseContainerConfigurationWithCache(FlexGridLayout* layoutManager, const std::string& config) {
    if (!layoutManager || config.empty()) return;
    
    // Check cache first
    auto cacheIter = containerConfigCache.find(config);
    std::vector<std::pair<std::string, std::string>> parsedConfig;
    
    if (cacheIter != containerConfigCache.end()) {
        parsedConfig = cacheIter->second;
    } else {
        // Parse and cache the configuration
        parsedConfig = parseContainerConfigurationString(config);
        containerConfigCache[config] = parsedConfig;
    }
    
    // Apply parsed configuration to layout manager
    applyContainerConfiguration(layoutManager, parsedConfig);
}

/**
 * Enhanced entity constraints parsing with more features
 */
void parseEntityConstraintsString(EntityConstraints* constraints, const std::string& config) {
    if (!constraints || config.empty()) return;
    
    std::vector<std::string> tokens = split(config, ',');
    
    for (const std::string& token : tokens) {
        std::string trimmed = token;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) continue;
        
        std::vector<std::string> parts = split(trimmed, ' ');
        if (parts.empty()) continue;
        
        std::string key = toLowerCase(parts[0]);
        
        if (key == "width") {
            if (parts.size() > 1) {
                try {
                    std::string widthValue = parts[1];
                    if (widthValue.find(':') != std::string::npos) {
                        // Range format: min:preferred:max
                        std::vector<std::string> rangeParts = split(widthValue, ':');
                        if (rangeParts.size() == 3) {
                            float min = parseFloat(rangeParts[0]);
                            float preferred = parseFloat(rangeParts[1]);
                            float max = parseFloat(rangeParts[2]);
                            constraints->setWidth(SizeConstraint::range(min, preferred, max));
                        }
                    } else if (widthValue.back() == '%') {
                        // Percentage
                        float percent = parseFloat(widthValue.substr(0, widthValue.length() - 1));
                        constraints->setWidth(SizeConstraint::percentage(percent));
                    } else {
                        // Fixed size
                        float width = parseFloat(widthValue);
                        constraints->setWidth(SizeConstraint::fixed(width));
                    }
                } catch (...) {
                    // Ignore invalid width values
                }
            }
        }
        else if (key == "height") {
            if (parts.size() > 1) {
                try {
                    std::string heightValue = parts[1];
                    if (heightValue.find(':') != std::string::npos) {
                        // Range format: min:preferred:max
                        std::vector<std::string> rangeParts = split(heightValue, ':');
                        if (rangeParts.size() == 3) {
                            float min = parseFloat(rangeParts[0]);
                            float preferred = parseFloat(rangeParts[1]);
                            float max = parseFloat(rangeParts[2]);
                            constraints->setHeight(SizeConstraint::range(min, preferred, max));
                        }
                    } else if (heightValue.back() == '%') {
                        // Percentage
                        float percent = parseFloat(heightValue.substr(0, heightValue.length() - 1));
                        constraints->setHeight(SizeConstraint::percentage(percent));
                    } else {
                        // Fixed size
                        float height = parseFloat(heightValue);
                        constraints->setHeight(SizeConstraint::fixed(height));
                    }
                } catch (...) {
                    // Ignore invalid height values
                }
            }
        }
        else if (key == "minwidth") {
            if (parts.size() > 1) {
                try {
                    float minWidth = parseFloat(parts[1]);
                    constraints->setMinWidth(SizeConstraint::fixed(minWidth));
                } catch (...) {
                    // Ignore invalid values
                }
            }
        }
        else if (key == "maxwidth") {
            if (parts.size() > 1) {
                try {
                    float maxWidth = parseFloat(parts[1]);
                    constraints->setMaxWidth(SizeConstraint::fixed(maxWidth));
                } catch (...) {
                    // Ignore invalid values
                }
            }
        }
        else if (key == "minheight") {
            if (parts.size() > 1) {
                try {
                    float minHeight = parseFloat(parts[1]);
                    constraints->setMinHeight(SizeConstraint::fixed(minHeight));
                } catch (...) {
                    // Ignore invalid values
                }
            }
        }
        else if (key == "maxheight") {
            if (parts.size() > 1) {
                try {
                    float maxHeight = parseFloat(parts[1]);
                    constraints->setMaxHeight(SizeConstraint::fixed(maxHeight));
                } catch (...) {
                    // Ignore invalid values
                }
            }
        }
        else if (key == "grow") {
            float growValue = 1.0f;
            if (parts.size() > 1) {
                try {
                    growValue = parseFloat(parts[1]);
                } catch (...) {
                    growValue = 1.0f;
                }
            }
            constraints->setGrowX(growValue)->setGrowY(growValue);
        }
        else if (key == "growx") {
            float growValue = 1.0f;
            if (parts.size() > 1) {
                try {
                    growValue = parseFloat(parts[1]);
                } catch (...) {
                    growValue = 1.0f;
                }
            }
            constraints->setGrowX(growValue);
        }
        else if (key == "growy") {
            float growValue = 1.0f;
            if (parts.size() > 1) {
                try {
                    growValue = parseFloat(parts[1]);
                } catch (...) {
                    growValue = 1.0f;
                }
            }
            constraints->setGrowY(growValue);
        }
        else if (key == "growprio" || key == "growpriority") {
            int priority = 100;
            if (parts.size() > 1) {
                try {
                    priority = static_cast<int>(parseFloat(parts[1]));
                } catch (...) {
                    priority = 100;
                }
            }
            constraints->setGrowPriorityX(priority)->setGrowPriorityY(priority);
        }
        else if (key == "shrink") {
            float shrinkValue = 100.0f;
            if (parts.size() > 1) {
                try {
                    shrinkValue = parseFloat(parts[1]);
                } catch (...) {
                    shrinkValue = 100.0f;
                }
            }
            constraints->setShrinkX(shrinkValue)->setShrinkY(shrinkValue);
        }
        else if (key == "shrinkprio" || key == "shrinkpriority") {
            int priority = 100;
            if (parts.size() > 1) {
                try {
                    priority = static_cast<int>(parseFloat(parts[1]));
                } catch (...) {
                    priority = 100;
                }
            }
            constraints->setShrinkPriorityX(priority)->setShrinkPriorityY(priority);
        }
        else if (key == "span" || key == "spanx") {
            if (parts.size() > 1) {
                try {
                    int span = static_cast<int>(parseFloat(parts[1]));
                    constraints->setSpanX(span);
                    if (parts.size() > 2) {
                        int spanY = static_cast<int>(parseFloat(parts[2]));
                        constraints->setSpanY(spanY);
                    }
                } catch (...) {
                    // Ignore invalid span values
                }
            }
        }
        else if (key == "spany") {
            if (parts.size() > 1) {
                try {
                    int spanY = static_cast<int>(parseFloat(parts[1]));
                    constraints->setSpanY(spanY);
                } catch (...) {
                    // Ignore invalid span values
                }
            }
        }
        else if (key == "wrap") {
            constraints->setWrap(true);
        }
        else if (key == "newline") {
            constraints->setNewline(true);
        }
        else if (key == "skip") {
            int skipCount = 1;
            if (parts.size() > 1) {
                try {
                    skipCount = static_cast<int>(parseFloat(parts[1]));
                } catch (...) {
                    skipCount = 1;
                }
            }
            constraints->setSkip(skipCount);
        }
        else if (key == "split") {
            int splitCount = 2;
            if (parts.size() > 1) {
                try {
                    splitCount = static_cast<int>(parseFloat(parts[1]));
                } catch (...) {
                    splitCount = 2;
                }
            }
            constraints->setSplit(splitCount);
        }
        else if (key == "cell") {
            if (parts.size() > 2) {
                try {
                    int cellX = static_cast<int>(parseFloat(parts[1]));
                    int cellY = static_cast<int>(parseFloat(parts[2]));
                    constraints->setCellX(cellX)->setCellY(cellY);
                } catch (...) {
                    // Ignore invalid cell values
                }
            }
        }
        else if (key == "align") {
            if (parts.size() > 1) {
                std::string alignValue = toLowerCase(parts[1]);
                if (alignValue == "left" || alignValue == "start") {
                    constraints->setHorizontalAlign(Alignment::Start);
                } else if (alignValue == "center") {
                    constraints->setHorizontalAlign(Alignment::Center);
                } else if (alignValue == "right" || alignValue == "end") {
                    constraints->setHorizontalAlign(Alignment::End);
                } else if (alignValue == "fill") {
                    constraints->setHorizontalAlign(Alignment::Fill);
                } else if (alignValue == "top") {
                    constraints->setVerticalAlign(Alignment::Start);
                } else if (alignValue == "middle") {
                    constraints->setVerticalAlign(Alignment::Center);
                } else if (alignValue == "bottom") {
                    constraints->setVerticalAlign(Alignment::End);
                } else if (alignValue == "baseline") {
                    constraints->setVerticalAlign(Alignment::Baseline);
                }
                
                // Check for second alignment value
                if (parts.size() > 2) {
                    std::string vAlignValue = toLowerCase(parts[2]);
                    if (vAlignValue == "top" || vAlignValue == "start") {
                        constraints->setVerticalAlign(Alignment::Start);
                    } else if (vAlignValue == "center" || vAlignValue == "middle") {
                        constraints->setVerticalAlign(Alignment::Center);
                    } else if (vAlignValue == "bottom" || vAlignValue == "end") {
                        constraints->setVerticalAlign(Alignment::End);
                    } else if (vAlignValue == "fill") {
                        constraints->setVerticalAlign(Alignment::Fill);
                    } else if (vAlignValue == "baseline") {
                        constraints->setVerticalAlign(Alignment::Baseline);
                    }
                }
            }
        }
        else if (key == "dock") {
            if (parts.size() > 1) {
                std::string dockValue = toLowerCase(parts[1]);
                if (dockValue == "north" || dockValue == "top") {
                    constraints->setBorderAttachment(BorderSide::Top);
                } else if (dockValue == "south" || dockValue == "bottom") {
                    constraints->setBorderAttachment(BorderSide::Bottom);
                } else if (dockValue == "west" || dockValue == "left") {
                    constraints->setBorderAttachment(BorderSide::Left);
                } else if (dockValue == "east" || dockValue == "right") {
                    constraints->setBorderAttachment(BorderSide::Right);
                }
            }
        }
        else if (key == "pos" || key == "position") {
            if (parts.size() > 2) {
                try {
                    float x = parseFloat(parts[1]);
                    float y = parseFloat(parts[2]);
                    constraints->setAbsolutePositioning(true)
                              ->setAbsoluteX(x)
                              ->setAbsoluteY(y);
                    
                    if (parts.size() > 4) {
                        float x2 = parseFloat(parts[3]);
                        float y2 = parseFloat(parts[4]);
                        constraints->setAbsoluteX2(x2)->setAbsoluteY2(y2);
                    }
                } catch (...) {
                    // Ignore invalid position values
                }
            }
        }
        else if (key == "sg" || key == "sizegroup") {
            if (parts.size() > 1) {
                constraints->setSizeGroup(parts[1]);
            }
        }
        else if (key == "eg" || key == "endgroup") {
            if (parts.size() > 1) {
                constraints->setEndGroup(parts[1]);
            }
        }
        else if (key == "id") {
            if (parts.size() > 1) {
                constraints->setComponentId(parts[1]);
            }
        }
        else if (key == "hidemode") {
            if (parts.size() > 1) {
                std::string hideModeValue = parts[1];
                if (hideModeValue == "0" || toLowerCase(hideModeValue) == "default") {
                    constraints->setHideMode(HideMode::Default);
                } else if (hideModeValue == "3" || toLowerCase(hideModeValue) == "exclude") {
                    constraints->setHideMode(HideMode::Exclude);
                }
            }
        }
        else if (key == "margin") {
            if (parts.size() > 1) {
                std::vector<std::string> marginParts = split(parts[1], ' ');
                if (marginParts.size() == 1) {
                    // All sides same
                    try {
                        float margin = parseFloat(marginParts[0]);
                        constraints->setMargin(Insets(margin));
                    } catch (...) {
                        // Ignore invalid margin values
                    }
                } else if (marginParts.size() == 4) {
                    // top, left, bottom, right
                    try {
                        float top = parseFloat(marginParts[0]);
                        float left = parseFloat(marginParts[1]);
                        float bottom = parseFloat(marginParts[2]);
                        float right = parseFloat(marginParts[3]);
                        constraints->setMargin(Insets(top, left, bottom, right));
                    } catch (...) {
                        // Ignore invalid margin values
                    }
                }
            }
        }
        else if (key == "pad" || key == "padding") {
            if (parts.size() > 1) {
                std::vector<std::string> paddingParts = split(parts[1], ' ');
                if (paddingParts.size() == 1) {
                    // All sides same
                    try {
                        float padding = parseFloat(paddingParts[0]);
                        constraints->setPadding(Insets(padding));
                    } catch (...) {
                        // Ignore invalid padding values
                    }
                } else if (paddingParts.size() == 4) {
                    // top, left, bottom, right
                    try {
                        float top = parseFloat(paddingParts[0]);
                        float left = parseFloat(paddingParts[1]);
                        float bottom = parseFloat(paddingParts[2]);
                        float right = parseFloat(paddingParts[3]);
                        constraints->setPadding(Insets(top, left, bottom, right));
                    } catch (...) {
                        // Ignore invalid padding values
                    }
                }
            }
        }
    }
}

/**
 * Parse entity constraints with caching
 */
void parseEntityConstraintsWithCache(EntityConstraints* constraints, const std::string& config) {
    if (!constraints || config.empty()) return;
    
    // Check cache first
    auto cacheIter = constraintCache.find(config);
    
    if (cacheIter != constraintCache.end()) {
        // Copy cached constraints
        *constraints = cacheIter->second;
    } else {
        // Parse, cache, and apply
        parseEntityConstraintsString(constraints, config);
        constraintCache[config] = *constraints;
    }
}

/**
 * Cache management functions
 */
void clearConstraintCache() {
    constraintCache.clear();
    containerConfigCache.clear();
}

size_t getConstraintCacheSize() {
    return constraintCache.size() + containerConfigCache.size();
}

//==============================================================================
// Legacy Phase 1 Functions (for backward compatibility)
//==============================================================================

/**
 * Parse container configuration string and apply to FlexGridLayout
 * Basic implementation for Phase 1 - will be enhanced with caching in Phase 2
 */
void parseContainerConfiguration(FlexGridLayout* layoutManager, const std::string& config) {
    if (!layoutManager || config.empty()) return;
    
    std::vector<std::string> tokens = split(config, ',');
    
    for (const std::string& token : tokens) {
        std::string trimmed = token;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) continue;
        
        std::vector<std::string> parts = split(trimmed, ' ');
        if (parts.empty()) continue;
        
        std::string key = toLowerCase(parts[0]);
        
        if (key == "gap") {
            float gap = 5.0f; // default
            if (parts.size() > 1) {
                try {
                    gap = parseFloat(parts[1]);
                } catch (...) {
                    gap = 5.0f;
                }
            }
            layoutManager->setGap(gap, gap);
        }
        else if (key == "fill") {
            layoutManager->setFill(true, true);
        }
        else if (key == "wrap") {
            int columns = -1; // auto-wrap
            if (parts.size() > 1) {
                try {
                    columns = static_cast<int>(parseFloat(parts[1]));
                } catch (...) {
                    columns = -1;
                }
            }
            layoutManager->setWrap(columns);
        }
        else if (key == "debug") {
            layoutManager->setDebugMode(true);
        }
        else if (key == "nogrid") {
            layoutManager->setNoGrid(true);
        }
        // Add more container options as needed
    }
}

/**
 * Parse entity constraint string and apply to EntityConstraints
 * Basic implementation for Phase 1 - will be enhanced with caching in Phase 2
 */
void parseEntityConstraints(EntityConstraints* constraints, const std::string& config) {
    if (!constraints || config.empty()) return;
    
    std::vector<std::string> tokens = split(config, ',');
    
    for (const std::string& token : tokens) {
        std::string trimmed = token;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        if (trimmed.empty()) continue;
        
        std::vector<std::string> parts = split(trimmed, ' ');
        if (parts.empty()) continue;
        
        std::string key = toLowerCase(parts[0]);
        
        if (key == "width") {
            if (parts.size() > 1) {
                try {
                    float width = parseFloat(parts[1]);
                    constraints->setWidth(SizeConstraint::fixed(width));
                } catch (...) {
                    // Ignore invalid width values
                }
            }
        }
        else if (key == "height") {
            if (parts.size() > 1) {
                try {
                    float height = parseFloat(parts[1]);
                    constraints->setHeight(SizeConstraint::fixed(height));
                } catch (...) {
                    // Ignore invalid height values
                }
            }
        }
        else if (key == "grow") {
            constraints->setGrowX(1.0f)->setGrowY(1.0f);
        }
        else if (key == "growx") {
            constraints->setGrowX(1.0f);
        }
        else if (key == "growy") {
            constraints->setGrowY(1.0f);
        }
        else if (key == "span") {
            if (parts.size() > 1) {
                try {
                    int span = static_cast<int>(parseFloat(parts[1]));
                    constraints->setSpanX(span);
                    if (parts.size() > 2) {
                        int spanY = static_cast<int>(parseFloat(parts[2]));
                        constraints->setSpanY(spanY);
                    }
                } catch (...) {
                    // Ignore invalid span values
                }
            }
        }
        else if (key == "wrap") {
            constraints->setWrap(true);
        }
        else if (key == "newline") {
            constraints->setNewline(true);
        }
        else if (key == "align") {
            if (parts.size() > 1) {
                std::string alignValue = toLowerCase(parts[1]);
                if (alignValue == "left" || alignValue == "start") {
                    constraints->setHorizontalAlign(Alignment::Start);
                } else if (alignValue == "center") {
                    constraints->setHorizontalAlign(Alignment::Center);
                } else if (alignValue == "right" || alignValue == "end") {
                    constraints->setHorizontalAlign(Alignment::End);
                } else if (alignValue == "fill") {
                    constraints->setHorizontalAlign(Alignment::Fill);
                }
            }
        }
        else if (key == "dock") {
            if (parts.size() > 1) {
                std::string dockValue = toLowerCase(parts[1]);
                if (dockValue == "north" || dockValue == "top") {
                    constraints->setBorderAttachment(BorderSide::Top);
                } else if (dockValue == "south" || dockValue == "bottom") {
                    constraints->setBorderAttachment(BorderSide::Bottom);
                } else if (dockValue == "west" || dockValue == "left") {
                    constraints->setBorderAttachment(BorderSide::Left);
                } else if (dockValue == "east" || dockValue == "right") {
                    constraints->setBorderAttachment(BorderSide::Right);
                }
            }
        }
        // Add more constraint options as needed
    }
}

} // namespace LayoutEngine
