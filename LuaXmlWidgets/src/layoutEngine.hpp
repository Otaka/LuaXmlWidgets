#ifndef layoutEngine_hpp
#define layoutEngine_hpp

//
// Generic Layout Engine for LuaXmlWidgets
// 
// Framework-independent layout engine with no external dependencies.
// This engine can be copy/pasted to other projects without modification.
// Inspired by MigLayout (Java) layout system.
//
// Core Design Principles:
// - Constraints Down, Sizes Up: Parents pass constraints, children report sizes
// - Framework Independence: Works with abstract geometric entities
// - Typed API: No string parsing in core engine
//

#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <stdexcept>

namespace LayoutEngine {

// Forward declarations
class LayoutEntity;
class FlexGridLayout;

//==============================================================================
// Core Data Structures
//==============================================================================

/**
 * Simple size representation (width, height)
 */
struct LayoutSize {
    float width = 0.0f;
    float height = 0.0f;
    
    LayoutSize() = default;
    LayoutSize(float w, float h) : width(w), height(h) {}
    
    bool isValid() const { return width >= 0.0f && height >= 0.0f; }
};

/**
 * Insets for margins, padding, and gaps
 */
struct Insets {
    float top = 0.0f;
    float left = 0.0f;
    float bottom = 0.0f;
    float right = 0.0f;
    
    Insets() = default;
    Insets(float all) : top(all), left(all), bottom(all), right(all) {}
    Insets(float vertical, float horizontal) : top(vertical), left(horizontal), bottom(vertical), right(horizontal) {}
    Insets(float t, float l, float b, float r) : top(t), left(l), bottom(b), right(r) {}
    
    float horizontalTotal() const { return left + right; }
    float verticalTotal() const { return top + bottom; }
};

/**
 * Size constraint with min/preferred/max values
 */
class SizeConstraint {
public:
    enum Type {
        FIXED,        // Fixed size
        RANGE,        // Min/preferred/max range
        PERCENTAGE,   // Percentage of container
        CONTENT       // Size based on content
    };
    
private:
    Type type_;
    float min_ = 0.0f;
    float preferred_ = 0.0f;
    float max_ = std::numeric_limits<float>::max();
    
public:
    SizeConstraint() : type_(CONTENT) {}
    
    static SizeConstraint fixed(float size) {
        SizeConstraint sc;
        sc.type_ = FIXED;
        sc.min_ = sc.preferred_ = sc.max_ = size;
        return sc;
    }
    
    static SizeConstraint range(float min, float preferred, float max) {
        SizeConstraint sc;
        sc.type_ = RANGE;
        sc.min_ = min;
        sc.preferred_ = preferred;
        sc.max_ = max;
        return sc;
    }
    
    static SizeConstraint percentage(float percent) {
        SizeConstraint sc;
        sc.type_ = PERCENTAGE;
        sc.preferred_ = percent;
        return sc;
    }
    
    static SizeConstraint content() {
        SizeConstraint sc;
        sc.type_ = CONTENT;
        return sc;
    }
    
    Type getType() const { return type_; }
    float getMin() const { return min_; }
    float getPreferred() const { return preferred_; }
    float getMax() const { return max_; }
    
    float calculateSize(float containerSize, float contentSize) const;
};

/**
 * Layout constraints passed from parent to child
 */
class LayoutConstraints {
    float maxWidth = std::numeric_limits<float>::max();
    float maxHeight = std::numeric_limits<float>::max();
    float minWidth = 0.0f;
    float minHeight = 0.0f;
public:
    LayoutConstraints() = default;
    LayoutConstraints(float maxW, float maxH) : maxWidth(maxW), maxHeight(maxH) {}
    LayoutConstraints(float minW, float minH, float maxW, float maxH) 
        : maxWidth(maxW), maxHeight(maxH), minWidth(minW), minHeight(minH) {}
    
    bool isValid() const {
        return maxWidth >= minWidth && maxHeight >= minHeight && 
               maxWidth >= 0.0f && maxHeight >= 0.0f &&
               minWidth >= 0.0f && minHeight >= 0.0f;
    }
    float getMaxWidth() const { return maxWidth; }
    float getMaxHeight() const { return maxHeight; }
    float getMinWidth() const { return minWidth; }
    float getMinHeight() const { return minHeight; }
    LayoutConstraints* setMaxWidth(float w) { maxWidth = w; return this; }
    LayoutConstraints* setMaxHeight(float h) { maxHeight = h; return this; }
    LayoutConstraints* setMinWidth(float w) { minWidth = w; return this; }
    LayoutConstraints* setMinHeight(float h) { minHeight = h; return this; }
    LayoutConstraints* setMinWidthHeight(float w, float h) { minWidth = w; minHeight = h; return this; }
    LayoutConstraints* setMaxWidthHeight(float w, float h) { maxWidth = w; maxHeight = h; return this; }
};

/**
 * Alignment options
 */
enum class Alignment {
    Start=0,      // Left/Top
    Center=1,     // Center
    End=2,        // Right/Bottom
    Fill=3,       // Stretch to fill
    Baseline=4    // Text baseline (for text components)
};

/**
 * Flow direction for layouts
 */
enum class FlowDirection {
    Default,     // Use container default
    Horizontal,  // Left to right
    Vertical     // Top to bottom
};

/**
 * Border sides for docking
 */
enum class BorderSide {
    None,
    Top,
    Bottom, 
    Left,
    Right
};

/**
 * Hide modes for invisible components
 */
enum class HideMode {
    Default,    // Mode 0: Invisible components treated as visible (reserve space)
    Exclude     // Mode 3: Don't participate in layout at all
};

/**
 * Entity constraints for individual components - Now with builder pattern
 */
class EntityConstraints {
private:
    // Sizing
    SizeConstraint width_ = SizeConstraint::content();
    SizeConstraint height_ = SizeConstraint::content();
    SizeConstraint minWidth_;
    SizeConstraint maxWidth_;
    SizeConstraint minHeight_;
    SizeConstraint maxHeight_;
    
    // Positioning
    Alignment horizontalAlign_ = Alignment::Center;
    Alignment verticalAlign_ = Alignment::Center;
    
    // Grid behavior
    int spanX_ = 1;
    int spanY_ = 1;
    float growX_ = 0.0f;
    float growY_ = 0.0f;
    int growPriorityX_ = 100;
    int growPriorityY_ = 100;
    float shrinkX_ = 100.0f;
    float shrinkY_ = 100.0f;
    int shrinkPriorityX_ = 100;
    int shrinkPriorityY_ = 100;
    
    // Flow control
    bool wrap_ = false;
    bool newline_ = false;
    int skip_ = 0;
    int split_ = 0;
    FlowDirection cellFlow_ = FlowDirection::Default;
    
    // Grid positioning
    int cellX_ = -1; // -1 means auto
    int cellY_ = -1; // -1 means auto
    
    // Spacing
    Insets margin_;
    Insets gap_;
    Insets padding_;
    
    // Grouping
    std::string sizeGroup_;
    std::string endGroup_;
    std::string componentId_;
    
    // Positioning modes
    BorderSide borderAttachment_ = BorderSide::None;
    bool absolutePositioning_ = false;
    float absoluteX_ = 0.0f;
    float absoluteY_ = 0.0f;
    float absoluteX2_ = -1.0f; // -1 means not set
    float absoluteY2_ = -1.0f; // -1 means not set
    
    // Visibility behavior
    HideMode hideMode_ = HideMode::Default;

public:
    // Getters for all properties
    SizeConstraint getWidth() const { return width_; }
    SizeConstraint getHeight() const { return height_; }
    SizeConstraint getMinWidth() const { return minWidth_; }
    SizeConstraint getMaxWidth() const { return maxWidth_; }
    SizeConstraint getMinHeight() const { return minHeight_; }
    SizeConstraint getMaxHeight() const { return maxHeight_; }
    
    Alignment getHorizontalAlign() const { return horizontalAlign_; }
    Alignment getVerticalAlign() const { return verticalAlign_; }
    
    int getSpanX() const { return spanX_; }
    int getSpanY() const { return spanY_; }
    float getGrowX() const { return growX_; }
    float getGrowY() const { return growY_; }
    int getGrowPriorityX() const { return growPriorityX_; }
    int getGrowPriorityY() const { return growPriorityY_; }
    float getShrinkX() const { return shrinkX_; }
    float getShrinkY() const { return shrinkY_; }
    int getShrinkPriorityX() const { return shrinkPriorityX_; }
    int getShrinkPriorityY() const { return shrinkPriorityY_; }
    
    bool getWrap() const { return wrap_; }
    bool getNewline() const { return newline_; }
    int getSkip() const { return skip_; }
    int getSplit() const { return split_; }
    FlowDirection getCellFlow() const { return cellFlow_; }
    
    int getCellX() const { return cellX_; }
    int getCellY() const { return cellY_; }
    
    const Insets& getMargin() const { return margin_; }
    const Insets& getGap() const { return gap_; }
    const Insets& getPadding() const { return padding_; }
    
    const std::string& getSizeGroup() const { return sizeGroup_; }
    const std::string& getEndGroup() const { return endGroup_; }
    const std::string& getComponentId() const { return componentId_; }
    
    BorderSide getBorderAttachment() const { return borderAttachment_; }
    bool getAbsolutePositioning() const { return absolutePositioning_; }
    float getAbsoluteX() const { return absoluteX_; }
    float getAbsoluteY() const { return absoluteY_; }
    float getAbsoluteX2() const { return absoluteX2_; }
    float getAbsoluteY2() const { return absoluteY2_; }
    
    HideMode getHideMode() const { return hideMode_; }

    // Fluent interface setters
    EntityConstraints* setWidth(const SizeConstraint& width) { width_ = width; return this; }
    EntityConstraints* setHeight(const SizeConstraint& height) { height_ = height; return this; }
    EntityConstraints* setMinWidth(const SizeConstraint& minWidth) { minWidth_ = minWidth; return this; }
    EntityConstraints* setMaxWidth(const SizeConstraint& maxWidth) { maxWidth_ = maxWidth; return this; }
    EntityConstraints* setMinHeight(const SizeConstraint& minHeight) { minHeight_ = minHeight; return this; }
    EntityConstraints* setMaxHeight(const SizeConstraint& maxHeight) { maxHeight_ = maxHeight; return this; }
    
    EntityConstraints* setHorizontalAlign(Alignment align) { horizontalAlign_ = align; return this; }
    EntityConstraints* setVerticalAlign(Alignment align) { verticalAlign_ = align; return this; }
    
    EntityConstraints* setSpanX(int span) { spanX_ = span; return this; }
    EntityConstraints* setSpanY(int span) { spanY_ = span; return this; }
    EntityConstraints* setGrowX(float grow) { growX_ = grow; return this; }
    EntityConstraints* setGrowY(float grow) { growY_ = grow; return this; }
    EntityConstraints* setGrowPriorityX(int priority) { growPriorityX_ = priority; return this; }
    EntityConstraints* setGrowPriorityY(int priority) { growPriorityY_ = priority; return this; }
    EntityConstraints* setShrinkX(float shrink) { shrinkX_ = shrink; return this; }
    EntityConstraints* setShrinkY(float shrink) { shrinkY_ = shrink; return this; }
    EntityConstraints* setShrinkPriorityX(int priority) { shrinkPriorityX_ = priority; return this; }
    EntityConstraints* setShrinkPriorityY(int priority) { shrinkPriorityY_ = priority; return this; }
    
    EntityConstraints* setWrap(bool wrap) { wrap_ = wrap; return this; }
    EntityConstraints* setNewline(bool newline) { newline_ = newline; return this; }
    EntityConstraints* setSkip(int skip) { skip_ = skip; return this; }
    EntityConstraints* setSplit(int split) { split_ = split; return this; }
    EntityConstraints* setCellFlow(FlowDirection flow) { cellFlow_ = flow; return this; }
    
    EntityConstraints* setCellX(int x) { cellX_ = x; return this; }
    EntityConstraints* setCellY(int y) { cellY_ = y; return this; }
    
    EntityConstraints* setMargin(const Insets& margin) { margin_ = margin; return this; }
    EntityConstraints* setGap(const Insets& gap) { gap_ = gap; return this; }
    EntityConstraints* setPadding(const Insets& padding) { padding_ = padding; return this; }
    
    EntityConstraints* setSizeGroup(const std::string& group) { sizeGroup_ = group; return this; }
    EntityConstraints* setEndGroup(const std::string& group) { endGroup_ = group; return this; }
    EntityConstraints* setComponentId(const std::string& id) { componentId_ = id; return this; }
    
    EntityConstraints* setBorderAttachment(BorderSide side) { borderAttachment_ = side; return this; }
    EntityConstraints* setAbsolutePositioning(bool absolute) { absolutePositioning_ = absolute; return this; }
    EntityConstraints* setAbsoluteX(float x) { absoluteX_ = x; return this; }
    EntityConstraints* setAbsoluteY(float y) { absoluteY_ = y; return this; }
    EntityConstraints* setAbsoluteX2(float x2) { absoluteX2_ = x2; return this; }
    EntityConstraints* setAbsoluteY2(float y2) { absoluteY2_ = y2; return this; }
    
    EntityConstraints* setHideMode(HideMode mode) { hideMode_ = mode; return this; }
};

class LayoutEntity {
public:
    // Callback function type for position and size updates
    using UpdateCallback = std::function<void(float x, float y, float width, float height)>;

private:
    LayoutSize size_;          // Current size
    float x_ = 0.0f;           // Current X position
    float y_ = 0.0f;           // Current Y position
    bool visible_ = true;      // Visibility state
    std::string name_;
    
    // Component properties
    LayoutSize preferredSize_ = {10.0f, 10.0f};  // Default preferred size
    UpdateCallback updateCallback_ = nullptr;      // Callback for position/size changes
    
public:
    LayoutEntity() = default;
    LayoutEntity(float preferredWidth, float preferredHeight) 
        : preferredSize_({preferredWidth, preferredHeight}) {}
    
    std::string getName() const { return name_; }
    
    // Size and position getters
    float getX() const { return x_; }
    float getY() const { return y_; }
    LayoutSize getSize() const { return size_; }
    LayoutSize getPreferredSize() const { return preferredSize_; }
    
    // Name and size and position setters with fluent interface
    LayoutEntity* setName(std::string name) { name_ = name; return this; }
    LayoutEntity* setX(float x) { x_ = x; return this; }
    LayoutEntity* setY(float y) { y_ = y; return this; }
    LayoutEntity* setPosition(float x, float y);
    LayoutEntity* setSize(const LayoutSize& size);
    LayoutEntity* setSize(float width, float height);
    
    // Callback management
    LayoutEntity* setUpdateCallback(UpdateCallback callback) {
        updateCallback_ = callback;
        return this;
    }
    
    // Visibility management
    bool isVisible() const { return visible_; }
    LayoutEntity* setVisible(bool visible) { visible_ = visible; return this; }
    
    // Preferred size management
    LayoutEntity* setPreferredSize(float width, float height) { 
        preferredSize_.width = width; 
        preferredSize_.height = height;
        return this;
    }
    
    LayoutEntity* setPreferredSize(const LayoutSize& size) { 
        preferredSize_ = size;
        return this;
    }
    
    // Calculate size based on constraints
    LayoutSize calculateSize(const LayoutConstraints& constraints);
};

//==============================================================================
// Layout Engine
//==============================================================================

struct EntityInfo {
    LayoutEntity* entity;
    EntityConstraints* constraints;
    LayoutSize calculatedSize;
    float x = 0.0f;
    float y = 0.0f;
    int gridX = -1;
    int gridY = -1;
};

/**
 * Main layout manager implementing MigLayout-style grid and flow layouts
 */
class FlexGridLayout {
private:
    // Container configuration
    float horizontalGap_ = 5.0f;
    float verticalGap_ = 5.0f;
    int wrapColumns_ = -1; // -1 for auto-wrap
    bool fillHorizontal_ = false;
    bool fillVertical_ = false;
    Insets containerInsets_;
    FlowDirection flowDirection_ = FlowDirection::Horizontal;
    bool debugMode_ = false;
    bool noGrid_ = false;
    Alignment containerHorizontalAlign_ = Alignment::Fill;
    Alignment containerVerticalAlign_ = Alignment::Fill;
    HideMode defaultHideMode_ = HideMode::Default;
    
    // Entity management
    std::vector<EntityInfo> entities_;
    std::unordered_map<std::string, LayoutEntity*> entityIds_;
    std::unordered_map<std::string, std::vector<LayoutEntity*>> sizeGroups_;
    std::unordered_map<std::string, std::vector<LayoutEntity*>> endGroups_;
    
    // Grid state
    int gridWidth_ = 0;
    int gridHeight_ = 0;
    std::vector<float> columnWidths_;
    std::vector<float> rowHeights_;
    std::vector<float> columnGrowWeights_;
    std::vector<float> rowGrowWeights_;
    
    // Last available space used in layout
    LayoutConstraints lastAvailableSpace_;

public:
    FlexGridLayout() = default;
    ~FlexGridLayout() = default;
    
    // Container configuration with fluent interface
    FlexGridLayout* setGap(float horizontal, float vertical) {
        horizontalGap_ = horizontal;
        verticalGap_ = vertical;
        return this;
    }
    
    FlexGridLayout* setWrap(int maxColumns = -1) {
        wrapColumns_ = maxColumns;
        return this;
    }
    
    FlexGridLayout* setFill(bool horizontal, bool vertical) {
        fillHorizontal_ = horizontal;
        fillVertical_ = vertical;
        return this;
    }
    
    FlexGridLayout* setInsets(float top, float left, float bottom, float right) {
        containerInsets_ = Insets(top, left, bottom, right);
        return this;
    }
    
    FlexGridLayout* setInsets(const Insets& insets) {
        containerInsets_ = insets;
        return this;
    }
    
    FlexGridLayout* setFlowDirection(FlowDirection direction) {
        flowDirection_ = direction;
        return this;
    }
    
    FlexGridLayout* setDebugMode(bool enabled) {
        debugMode_ = enabled;
        return this;
    }
    
    FlexGridLayout* setNoGrid(bool enabled) {
        noGrid_ = enabled;
        return this;
    }
    
    FlexGridLayout* setAlignment(Alignment horizontal, Alignment vertical) {
        containerHorizontalAlign_ = horizontal;
        containerVerticalAlign_ = vertical;
        return this;
    }
    
    FlexGridLayout* setHideMode(HideMode mode) {
        defaultHideMode_ = mode;
        return this;
    }
    
    // Entity management with fluent interface
    FlexGridLayout* addEntity(LayoutEntity* entity, EntityConstraints* constraints = nullptr);
    FlexGridLayout* removeEntity(LayoutEntity* entity);
    FlexGridLayout* setEntityConstraints(LayoutEntity* entity, EntityConstraints* constraints);
    FlexGridLayout* clearEntities();
    
    // Component identification and grouping
    FlexGridLayout* setEntityId(LayoutEntity* entity, const std::string& id);
    LayoutEntity* getEntityById(const std::string& id);
    FlexGridLayout* addToSizeGroup(LayoutEntity* entity, const std::string& groupName);
    FlexGridLayout* addToEndGroup(LayoutEntity* entity, const std::string& groupName);
    FlexGridLayout* removeFromSizeGroup(LayoutEntity* entity, const std::string& groupName);
    FlexGridLayout* removeFromEndGroup(LayoutEntity* entity, const std::string& groupName);
    void validateConstraints()const;
    
    // Layout execution
    LayoutSize performLayout(const LayoutConstraints& availableSpace);
    
    // Debug and inspection
    std::string getLayoutDebugInfo(bool printGridLayout, bool printLastAvailableSpace) const;
    
    // Entity access for visualization
    const std::vector<EntityInfo>& getEntities() const { return entities_; }
    std::vector<LayoutEntity*> getEntityList() const {
        std::vector<LayoutEntity*> result;
        for (const auto& info : entities_) {
            if (info.entity) {
                result.push_back(info.entity);
            }
        }
        return result;
    }

private:
    // Internal layout methods
    void calculateGrid();
    void calculateEntitySizes(const LayoutConstraints& availableSpace);
    void calculateColumnAndRowSizes(const LayoutConstraints& availableSpace);
    void positionEntities();
    void applySizeGroups();
    void applyEndGroups();
    
    // Helper methods
    EntityInfo* findEntityInfo(LayoutEntity* entity);
    bool shouldParticipateInLayout(const EntityInfo& info) const;
    LayoutConstraints createEntityConstraints(const EntityInfo& info, const LayoutConstraints& containerConstraints) const;
    void updateGridDimensions();
};

//==============================================================================
// String Parsing Functions
//==============================================================================

/**
 * Exception thrown when constraint parsing fails
 */
class ConstraintParseException : public std::runtime_error {
public:
    ConstraintParseException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * Parses layout constraint string and returns a configured FlexGridLayout
 * Example: "wrap 3, gap 10px 5px, insets 20, fill, debug"
 * @throws ConstraintParseException on invalid syntax
 */
FlexGridLayout* parseLayoutConstraints(const std::string& constraintStr);

/**
 * Parses entity constraint string and returns a configured EntityConstraints
 * Example: "width 100px!, grow, span 2, alignx fill, wrap"
 * @throws ConstraintParseException on invalid syntax
 */
EntityConstraints* parseEntityConstraints(const std::string& constraintStr);

/**
 * Parse container configuration string and apply to FlexGridLayout
 * Basic implementation for Phase 1 - will be enhanced with caching in Phase 2
 */
void parseContainerConfiguration(FlexGridLayout* layoutManager, const std::string& config);

/**
 * Parse entity constraint string and apply to EntityConstraints
 * Basic implementation for Phase 1 - will be enhanced with caching in Phase 2
 */
void parseEntityConstraints(EntityConstraints* constraints, const std::string& config);

} // namespace LayoutEngine

#endif
