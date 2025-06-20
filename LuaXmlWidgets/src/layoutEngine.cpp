#include "layoutEngine.hpp"
#include <algorithm>
#include <limits>
#include <sstream>
#include <cmath>

namespace LayoutEngine {

//==============================================================================
// Method implementations moved from header file
//==============================================================================

// SizeConstraint implementation
float SizeConstraint::calculateSize(float containerSize, float contentSize) const {
    switch (type_) {
        case FIXED:
            return preferred_;
        case RANGE:
            return std::max(min_, std::min(max_, preferred_));
        case PERCENTAGE:
            return containerSize * (preferred_ / 100.0f);
        case CONTENT:
            return contentSize;
    }
    return preferred_;
}

// LayoutEntity implementation
LayoutEntity* LayoutEntity::setPosition(float x, float y) { 
    x_ = x; 
    y_ = y; 
    
    // Notify callback if exists
    if (updateCallback_) {
        updateCallback_(x_, y_, size_.width, size_.height);
    }
    return this;
}

LayoutEntity* LayoutEntity::setSize(const LayoutSize& size) { 
    size_ = size; 
    
    // Notify callback if exists
    if (updateCallback_) {
        updateCallback_(x_, y_, size_.width, size_.height);
    }
    return this;
}

LayoutEntity* LayoutEntity::setSize(float width, float height) { 
    size_.width = width; 
    size_.height = height; 
    
    // Notify callback if exists
    if (updateCallback_) {
        updateCallback_(x_, y_, width, height);
    }
    return this;
}

LayoutSize LayoutEntity::calculateSize(const LayoutConstraints& constraints) {
    LayoutSize result = preferredSize_;
    
    // Adjust for maximum constraints - only if container is smaller than preferred
    result.width = std::min(result.width, constraints.getMaxWidth());
    result.height = std::min(result.height, constraints.getMaxHeight());
    
    // Ensure minimum sizes are respected
    result.width = std::max(result.width, constraints.getMinWidth());
    result.height = std::max(result.height, constraints.getMinHeight());
    
    return result;
}

// FlexGridLayout implementation for entity management
FlexGridLayout* FlexGridLayout::addEntity(LayoutEntity* entity, EntityConstraints* constraints) {
    if (!entity) return this;
    
    EntityInfo info;
    info.entity = entity;
    info.constraints = constraints ? constraints : new EntityConstraints();
    entities_.push_back(info);
    
    if (!constraints->getComponentId().empty()) {
        entityIds_[constraints->getComponentId()] = entity;
    }
    
    if (!constraints->getSizeGroup().empty()) {
        addToSizeGroup(entity, constraints->getSizeGroup());
    }
    
    if (!constraints->getEndGroup().empty()) {
        addToEndGroup(entity, constraints->getEndGroup());
    }
    
    return this;
}

FlexGridLayout* FlexGridLayout::removeEntity(LayoutEntity* entity) {
    auto it = std::find_if(entities_.begin(), entities_.end(), [entity](const EntityInfo& info) {
        return info.entity == entity;
    });
    
    if (it != entities_.end()) {
        // Remove from groups
        for (auto& pair : sizeGroups_) {
            auto& group = pair.second;
            group.erase(std::remove(group.begin(), group.end(), entity), group.end());
        }
        
        for (auto& pair : endGroups_) {
            auto& group = pair.second;
            group.erase(std::remove(group.begin(), group.end(), entity), group.end());
        }
        
        // Remove from id map
        for (auto it = entityIds_.begin(); it != entityIds_.end(); ) {
            if (it->second == entity) {
                it = entityIds_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Clean up constraints
        delete it->constraints;
        
        // Remove from entity list
        entities_.erase(it);
    }
    
    return this;
}

FlexGridLayout* FlexGridLayout::setEntityConstraints(LayoutEntity* entity, EntityConstraints* constraints) {
    if (!entity || !constraints) return this;
    
    auto* info = findEntityInfo(entity);
    if (info) {
        delete info->constraints;
        info->constraints = constraints;
    }
    return this;
}

FlexGridLayout* FlexGridLayout::clearEntities() {
    // Clean up constraints
    for (auto& info : entities_) {
        delete info.constraints;
    }
    
    entities_.clear();
    entityIds_.clear();
    sizeGroups_.clear();
    endGroups_.clear();
    return this;
}

// Component identification and grouping
FlexGridLayout* FlexGridLayout::setEntityId(LayoutEntity* entity, const std::string& id) {
    if (!id.empty()) {
        entityIds_[id] = entity;
        
        auto* info = findEntityInfo(entity);
        if (info && info->constraints) {
            info->constraints->setComponentId(id);
        }
    }
    return this;
}

LayoutEntity* FlexGridLayout::getEntityById(const std::string& id) {
    auto it = entityIds_.find(id);
    return it != entityIds_.end() ? it->second : nullptr;
}

FlexGridLayout* FlexGridLayout::addToSizeGroup(LayoutEntity* entity, const std::string& groupName) {
    if (!groupName.empty()) {
        auto& group = sizeGroups_[groupName];
        if (std::find(group.begin(), group.end(), entity) == group.end()) {
            group.push_back(entity);
        }
        
        auto* info = findEntityInfo(entity);
        if (info && info->constraints) {
            info->constraints->setSizeGroup(groupName);
        }
    }
    return this;
}

FlexGridLayout* FlexGridLayout::addToEndGroup(LayoutEntity* entity, const std::string& groupName) {
    if (!groupName.empty()) {
        auto& group = endGroups_[groupName];
        if (std::find(group.begin(), group.end(), entity) == group.end()) {
            group.push_back(entity);
        }
        
        auto* info = findEntityInfo(entity);
        if (info && info->constraints) {
            info->constraints->setEndGroup(groupName);
        }
    }
    return this;
}

FlexGridLayout* FlexGridLayout::removeFromSizeGroup(LayoutEntity* entity, const std::string& groupName) {
    auto it = sizeGroups_.find(groupName);
    if (it != sizeGroups_.end()) {
        auto& group = it->second;
        group.erase(std::remove(group.begin(), group.end(), entity), group.end());
    }
    return this;
}

FlexGridLayout* FlexGridLayout::removeFromEndGroup(LayoutEntity* entity, const std::string& groupName) {
    auto it = endGroups_.find(groupName);
    if (it != endGroups_.end()) {
        auto& group = it->second;
        group.erase(std::remove(group.begin(), group.end(), entity), group.end());
    }
    return this;
}

// Layout execution
LayoutSize FlexGridLayout::performLayout(const LayoutConstraints& availableSpace) {
    // Reset grid state
    gridWidth_ = 0;
    gridHeight_ = 0;
    columnWidths_.clear();
    rowHeights_.clear();
    columnGrowWeights_.clear();
    rowGrowWeights_.clear();
    
    // Store available space for later use
    lastAvailableSpace_ = availableSpace;
    
    // Calculate grid dimensions and initial sizes
    calculateGrid();
    calculateEntitySizes(availableSpace);
    
    // Apply size groups (components in same size group get same size)
    applySizeGroups();
    
    // Calculate column and row dimensions
    calculateColumnAndRowSizes(availableSpace);
    
    // Position entities
    positionEntities();
    
    // Apply end groups (components in same end group get aligned to same end position)
    applyEndGroups();
    
    // Calculate total size - this needs to account for border entities as well
    
    // Group entities by border side for size calculation
    float topBorderHeight = 0.0f;
    float bottomBorderHeight = 0.0f;
    float leftBorderWidth = 0.0f;
    float rightBorderWidth = 0.0f;
    
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        switch (info.constraints->getBorderAttachment()) {
            case BorderSide::Top:
                topBorderHeight = std::max(topBorderHeight, info.y + info.calculatedSize.height - containerInsets_.top);
                break;
            case BorderSide::Bottom:
                bottomBorderHeight = std::max(bottomBorderHeight, 
                    info.y + info.calculatedSize.height - (info.y - info.calculatedSize.height));
                break;
            case BorderSide::Left:
                leftBorderWidth = std::max(leftBorderWidth, info.calculatedSize.width);
                break;
            case BorderSide::Right:
                rightBorderWidth = std::max(rightBorderWidth, info.calculatedSize.width);
                break;
            default:
                break;
        }
    }
    
    // Add gaps for border areas if they exist
    if (leftBorderWidth > 0) leftBorderWidth += horizontalGap_;
    if (rightBorderWidth > 0) rightBorderWidth += horizontalGap_;
    if (topBorderHeight > 0) topBorderHeight += verticalGap_;
    if (bottomBorderHeight > 0) bottomBorderHeight += verticalGap_;
    
    // Calculate grid content size
    float gridWidth = containerInsets_.left + containerInsets_.right;
    float gridHeight = containerInsets_.top + containerInsets_.bottom;
    
    for (size_t i = 0; i < columnWidths_.size(); ++i) {
        gridWidth += columnWidths_[i];
        if (i < columnWidths_.size() - 1) {
            gridWidth += horizontalGap_;
        }
    }
    
    for (size_t i = 0; i < rowHeights_.size(); ++i) {
        gridHeight += rowHeights_[i];
        if (i < rowHeights_.size() - 1) {
            gridHeight += verticalGap_;
        }
    }
    
    // Total size includes grid content plus border regions
    float totalWidth = gridWidth + leftBorderWidth + rightBorderWidth;
    float totalHeight = gridHeight + topBorderHeight + bottomBorderHeight;
    
    return {totalWidth, totalHeight};
}

// Debug and inspection
std::string FlexGridLayout::getLayoutDebugInfo(bool printGridLayout, bool printLastAvailableSpace) const {
    std::stringstream ss;
    // Container properties
    ss << "Container:\n";
    ss << "  Grid size:" << gridWidth_ << "x" << gridHeight_ << "\n";
    
    // Grid layout information
    if (printGridLayout && (!columnWidths_.empty() || !rowHeights_.empty())) {
        ss << "\nGridLayout:\n";
        if (!columnWidths_.empty()) {
            ss << "  Column widths:";
            for (size_t i = 0; i < columnWidths_.size(); ++i) {
                if (i > 0) ss << ",";
                ss << columnWidths_[i];
            }
            ss << "\n";
        }
        if (!rowHeights_.empty()) {
            ss << "  Row heights:";
            for (size_t i = 0; i < rowHeights_.size(); ++i) {
                if (i > 0) ss << ",";
                ss << rowHeights_[i];
            }
            ss << "\n";
        }
        if (!columnGrowWeights_.empty()) {
            ss << "  Column grow weights:";
            for (size_t i = 0; i < columnGrowWeights_.size(); ++i) {
                if (i > 0) ss << ",";
                ss << columnGrowWeights_[i];
            }
            ss << "\n";
        }
        if (!rowGrowWeights_.empty()) {
            ss << "  Row grow weights:";
            for (size_t i = 0; i < rowGrowWeights_.size(); ++i) {
                if (i > 0) ss << ",";
                ss << rowGrowWeights_[i];
            }
            ss << "\n";
        }
    }
    
    if(printLastAvailableSpace) {
        // Last available space
        ss << "\nLastAvailableSpace:\n";
        ss << "  Width(min/max):" << lastAvailableSpace_.getMinWidth() << "-" << lastAvailableSpace_.getMaxWidth() << "\n";
        ss << "  Height(min/max):" << lastAvailableSpace_.getMinHeight() << "-" << lastAvailableSpace_.getMaxHeight() << "\n";
    }
    
    
    // Entities
    ss << "Entities:\n";
    for (size_t i = 0; i < entities_.size(); ++i) {
        const EntityInfo& info = entities_[i];
        const LayoutEntity* entity = info.entity;
        const EntityConstraints* constraints = info.constraints;
        if(i!=0){
            ss << "\n";
        }
        ss << "  index:" << i;
        // Entity name and basic info
        if (!entity->getName().empty()) {
            ss << " name:" << entity->getName();
        }
        if (!constraints->getComponentId().empty()) {
            ss << " id:" << constraints->getComponentId();
        }
        
        // Position and size
        ss << " XY:" << entity->getX() << "," << entity->getY();
        ss << " WH:" << entity->getSize().width << "x" << entity->getSize().height;
        ss << " CalcSize:" << info.calculatedSize.width << "x" << info.calculatedSize.height;
        ss << " Grid pos:" << info.gridX << "," << info.gridY;
    }

    
    // Entity IDs mapping
    if (!entityIds_.empty()) {
        ss << "\nEntityIdMapping:\n";
        for (const auto& pair : entityIds_) {
            size_t index=-1;
            for(size_t i = 0; i < entities_.size(); ++i){
                const EntityInfo& info = entities_[i];
                const LayoutEntity* entity = info.entity;
                if(pair.second==entity){
                    index=i;
                    break;
                }
            }
            
            ss << "  \"" << pair.first << "\"->" << index << "\n";
        }
    }

    
    return ss.str();
}

void FlexGridLayout::validateConstraints() const {
    // Validation logic
}

//==============================================================================
// Layout Engine Internal Implementation
//==============================================================================

// Implementations of private methods for FlexGridLayout class

void FlexGridLayout::calculateGrid() {
    // Determine the dimensions of the grid
    if (noGrid_) {
        gridWidth_ = 1;
        gridHeight_ = entities_.size();
        return;
    }
    
    gridWidth_ = wrapColumns_ > 0 ? wrapColumns_ : 1;
    gridHeight_ = 1;
    
    int col = 0;
    int row = 0;
    
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        // Skip border entities - they don't participate in grid layout
        if (info.constraints->getBorderAttachment() != BorderSide::None) {
            continue;
        }
        
        // Handle cell position if set explicitly
        if (info.constraints->getCellX() >= 0 && info.constraints->getCellY() >= 0) {
            info.gridX = info.constraints->getCellX();
            info.gridY = info.constraints->getCellY();
            gridWidth_ = std::max(gridWidth_, info.gridX + info.constraints->getSpanX());
            gridHeight_ = std::max(gridHeight_, info.gridY + info.constraints->getSpanY());
            continue;
        }
        
        // Handle newline
        if (info.constraints->getNewline() || 
            (wrapColumns_ > 0 && col >= wrapColumns_)) {
            col = 0;
            row++;
        }
        
        // Handle skip
        col += info.constraints->getSkip();
        
        // Assign grid position
        info.gridX = col;
        info.gridY = row;
        
        // Update grid dimensions
        gridWidth_ = std::max(gridWidth_, col + info.constraints->getSpanX());
        gridHeight_ = std::max(gridHeight_, row + info.constraints->getSpanY());
        
        // Update position for next component
        col += info.constraints->getSpanX();
    }
}

void FlexGridLayout::calculateEntitySizes(const LayoutConstraints& availableSpace) {
    // Calculate available space for content (without insets)
    float availableWidth = availableSpace.getMaxWidth() - (containerInsets_.left + containerInsets_.right);
    float availableHeight = availableSpace.getMaxHeight() - (containerInsets_.top + containerInsets_.bottom);
    
    // Calculate initial constraints for each entity
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        // Create constraints for the entity
        LayoutConstraints entityConstraints = createEntityConstraints(info, availableSpace);
        
        // Calculate entity size
        info.calculatedSize = info.entity->calculateSize(entityConstraints);
    }
}

void FlexGridLayout::calculateColumnAndRowSizes(const LayoutConstraints& availableSpace) {
    // Resize column and row arrays
    columnWidths_.resize(gridWidth_, 0.0f);
    rowHeights_.resize(gridHeight_, 0.0f);
    columnGrowWeights_.resize(gridWidth_, 0.0f);
    rowGrowWeights_.resize(gridHeight_, 0.0f);
    
    // First pass: find max dimensions of each column/row
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        // Skip border entities - they don't participate in grid layout
        if (info.constraints->getBorderAttachment() != BorderSide::None) {
            continue;
        }
        
        for (int x = 0; x < info.constraints->getSpanX(); x++) {
            int col = info.gridX + x;
            if (col >= 0 && col < gridWidth_) {
                // If entity spans multiple columns, divide its width
                float width = info.constraints->getSpanX() == 1 ? 
                    info.calculatedSize.width : (info.calculatedSize.width / info.constraints->getSpanX());
                    
                columnWidths_[col] = std::max(columnWidths_[col], width);
                
                // Only record growth weight if explicitly set to grow
                if (info.constraints->getGrowX() > 0) {
                    columnGrowWeights_[col] = std::max(columnGrowWeights_[col], info.constraints->getGrowX());
                }
            }
        }
        
        for (int y = 0; y < info.constraints->getSpanY(); y++) {
            int row = info.gridY + y;
            if (row >= 0 && row < gridHeight_) {
                // If entity spans multiple rows, divide its height
                float height = info.constraints->getSpanY() == 1 ? 
                    info.calculatedSize.height : (info.calculatedSize.height / info.constraints->getSpanY());
                    
                rowHeights_[row] = std::max(rowHeights_[row], height);
                
                // Only record growth weight if explicitly set to grow
                if (info.constraints->getGrowY() > 0) {
                    rowGrowWeights_[row] = std::max(rowGrowWeights_[row], info.constraints->getGrowY());
                }
            }
        }
    }
    
    // Second pass: grow columns/rows if space available and growth is enabled
    float totalWidth = 0.0f;
    float totalHeight = 0.0f;
    float totalGrowX = 0.0f;
    float totalGrowY = 0.0f;
    
    // Calculate total width/height and total grow weights
    for (size_t i = 0; i < columnWidths_.size(); i++) {
        totalWidth += columnWidths_[i];
        totalGrowX += columnGrowWeights_[i];
    }
    totalWidth += (columnWidths_.size() - 1) * horizontalGap_;
    
    for (size_t i = 0; i < rowHeights_.size(); i++) {
        totalHeight += rowHeights_[i];
        totalGrowY += rowGrowWeights_[i];
    }
    totalHeight += (rowHeights_.size() - 1) * verticalGap_;
    
    // Add insets
    float availableWidth = availableSpace.getMaxWidth() - (containerInsets_.left + containerInsets_.right);
    float availableHeight = availableSpace.getMaxHeight() - (containerInsets_.top + containerInsets_.bottom);
    
    // Distribute extra space based on growth weights - only if explicitly set to grow
    float extraWidth = availableWidth - totalWidth;
    float extraHeight = availableHeight - totalHeight;
    
    if (extraWidth > 0 && totalGrowX > 0) {
        for (size_t i = 0; i < columnWidths_.size(); i++) {
            if (columnGrowWeights_[i] > 0) {
                columnWidths_[i] += extraWidth * (columnGrowWeights_[i] / totalGrowX);
            }
        }
    }
    
    if (extraHeight > 0 && totalGrowY > 0) {
        for (size_t i = 0; i < rowHeights_.size(); i++) {
            if (rowGrowWeights_[i] > 0) {
                rowHeights_[i] += extraHeight * (rowGrowWeights_[i] / totalGrowY);
            }
        }
    }
}

void FlexGridLayout::positionEntities() {
    // Group entities by border side
    std::vector<EntityInfo*> topBorder;
    std::vector<EntityInfo*> bottomBorder;
    std::vector<EntityInfo*> leftBorder;
    std::vector<EntityInfo*> rightBorder;
    std::vector<EntityInfo*> normalEntities;
    
    // Use the stored available space
    LayoutConstraints& containerConstraints = lastAvailableSpace_;
    
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        // Group by border attachment
        switch (info.constraints->getBorderAttachment()) {
            case BorderSide::Top:
                topBorder.push_back(&info);
                break;
            case BorderSide::Bottom:
                bottomBorder.push_back(&info);
                break;
            case BorderSide::Left:
                leftBorder.push_back(&info);
                break;
            case BorderSide::Right:
                rightBorder.push_back(&info);
                break;
            case BorderSide::None:
                normalEntities.push_back(&info);
                break;
        }
    }
    
    // Calculate the starting position for the grid (after insets)
    float startX = containerInsets_.left;
    float startY = containerInsets_.top;
    float contentWidth = 0.0f;
    float contentHeight = 0.0f;
    
    // Calculate total grid width and height
    for (int i = 0; i < gridWidth_; i++) {
        contentWidth += columnWidths_[i];
        if (i < gridWidth_ - 1) contentWidth += horizontalGap_;
    }
    
    for (int i = 0; i < gridHeight_; i++) {
        contentHeight += rowHeights_[i];
        if (i < gridHeight_ - 1) contentHeight += verticalGap_;
    }
    
    // Handle left border components
    float leftBorderWidth = 0.0f;
    if (!leftBorder.empty()) {
        float leftX = startX;
        float leftY = startY;
        float maxLeftWidth = 0.0f;
        
        for (size_t i = 0; i < leftBorder.size(); ++i) {
            auto* info = leftBorder[i];
            
            // Set position
            info->entity->setPosition(leftX, leftY);
            info->entity->setSize(info->calculatedSize);
            
            // Save for later use
            info->x = leftX;
            info->y = leftY;
            
            // Update for next component - only add gap if not the last component
            leftY += info->calculatedSize.height;
            if (i < leftBorder.size() - 1) {
                leftY += verticalGap_;
            }
            maxLeftWidth = std::max(maxLeftWidth, info->calculatedSize.width);
        }
        
        leftBorderWidth = maxLeftWidth + horizontalGap_;
        startX += leftBorderWidth;
    }
    
    // Handle right border components
    float rightBorderWidth = 0.0f;
    if (!rightBorder.empty()) {
        float rightX = startX + contentWidth;
        float rightY = startY;
        float maxRightWidth = 0.0f;
        
        for (size_t i = 0; i < rightBorder.size(); ++i) {
            auto* info = rightBorder[i];
            
            // Set position
            info->entity->setPosition(rightX, rightY);
            info->entity->setSize(info->calculatedSize);
            
            // Save for later use
            info->x = rightX;
            info->y = rightY;
            
            // Update for next component - only add gap if not the last component
            rightY += info->calculatedSize.height;
            if (i < rightBorder.size() - 1) {
                rightY += verticalGap_;
            }
            maxRightWidth = std::max(maxRightWidth, info->calculatedSize.width);
        }
        
        rightBorderWidth = maxRightWidth + horizontalGap_;
    }
    
    // Handle top border components
    float topBorderHeight = 0.0f;
    if (!topBorder.empty()) {
        float topX = startX;
        float topY = startY;
        float maxTopHeight = 0.0f;
        float availableWidth = contentWidth;
        
        for (size_t i = 0; i < topBorder.size(); ++i) {
            auto* info = topBorder[i];
            
            // Calculate size based on fill constraints
            float width = info->calculatedSize.width;
            if (info->constraints->getHorizontalAlign() == Alignment::Fill) {
                width = availableWidth;
                info->calculatedSize.width = width;
            }
            
            // Set position
            info->entity->setPosition(topX, topY);
            info->entity->setSize({width, info->calculatedSize.height});
            
            // Save for later use
            info->x = topX;
            info->y = topY;
            
            // Update for next component - only add gap if not the last component
            topY += info->calculatedSize.height;
            if (i < topBorder.size() - 1) {
                topY += verticalGap_;
            }
            maxTopHeight = std::max(maxTopHeight, info->calculatedSize.height);
        }
        
        topBorderHeight = topY - startY;
        startY += topBorderHeight;
    }
    
    // Handle bottom border components
    float bottomBorderHeight = 0.0f;
    if (!bottomBorder.empty()) {
        float bottomY = startY + contentHeight + verticalGap_;
        float startBottomY = bottomY;
        float bottomX = startX;
        float availableWidth = contentWidth;
        
        for (size_t i = 0; i < bottomBorder.size(); ++i) {
            auto* info = bottomBorder[i];
            
            // Calculate size based on fill constraints
            float width = info->calculatedSize.width;
            if (info->constraints->getHorizontalAlign() == Alignment::Fill) {
                width = availableWidth;
                info->calculatedSize.width = width;
            }
            
            // Set position
            info->entity->setPosition(bottomX, bottomY);
            info->entity->setSize({width, info->calculatedSize.height});
            
            // Save for later use
            info->x = bottomX;
            info->y = bottomY;
            
            // Update for next component - only add gap if not the last component
            bottomY += info->calculatedSize.height;
            if (i < bottomBorder.size() - 1) {
                bottomY += verticalGap_;
            }
        }
        
        bottomBorderHeight = bottomY - startBottomY;
    }
    
    // Calculate column X positions (adjusted for borders)
    std::vector<float> columnX(gridWidth_);
    float x = startX;
    for (int i = 0; i < gridWidth_; i++) {
        columnX[i] = x;
        x += columnWidths_[i] + horizontalGap_;
    }
    
    // Calculate row Y positions (adjusted for borders)
    std::vector<float> rowY(gridHeight_);
    float y = startY;
    for (int i = 0; i < gridHeight_; i++) {
        rowY[i] = y;
        y += rowHeights_[i] + verticalGap_;
    }
    
    // Position each normal entity
    for (auto* info : normalEntities) {
        if (info->gridX < 0 || info->gridY < 0 || 
            info->gridX >= gridWidth_ || info->gridY >= gridHeight_) {
            continue;
        }
        
        // Calculate cell size
        float cellWidth = 0;
        float cellHeight = 0;
        
        for (int x = 0; x < info->constraints->getSpanX(); x++) {
            int col = info->gridX + x;
            if (col >= 0 && col < gridWidth_) {
                cellWidth += columnWidths_[col];
                if (x > 0) cellWidth += horizontalGap_;
            }
        }
        
        for (int y = 0; y < info->constraints->getSpanY(); y++) {
            int row = info->gridY + y;
            if (row >= 0 && row < gridHeight_) {
                cellHeight += rowHeights_[row];
                if (y > 0) cellHeight += verticalGap_;
            }
        }
        
        // Calculate entity size based on alignment and constraints
        LayoutSize entitySize = info->calculatedSize;
        
        // Handle horizontal sizing
        if (info->constraints->getHorizontalAlign() == Alignment::Fill) {
            // Fill alignment always fills the cell
            entitySize.width = cellWidth;
        } else if (info->constraints->getGrowX() > 0 && cellWidth > entitySize.width) {
            // Only grow if explicitly set to grow and cell is larger than current size
            float maxWidth = std::numeric_limits<float>::max();
            if (info->constraints->getMaxWidth().getType() != SizeConstraint::CONTENT) {
                maxWidth = info->constraints->getMaxWidth().calculateSize(containerConstraints.getMaxWidth(), 0);
            }
            // Grow up to the maximum size
            entitySize.width = std::min(cellWidth, maxWidth);
        }
        
        // Handle vertical sizing
        if (info->constraints->getVerticalAlign() == Alignment::Fill) {
            // Fill alignment always fills the cell
            entitySize.height = cellHeight;
        } else if (info->constraints->getGrowY() > 0 && cellHeight > entitySize.height) {
            // Only grow if explicitly set to grow and cell is larger than current size
            float maxHeight = std::numeric_limits<float>::max();
            if (info->constraints->getMaxHeight().getType() != SizeConstraint::CONTENT) {
                maxHeight = info->constraints->getMaxHeight().calculateSize(containerConstraints.getMaxHeight(), 0);
            }
            // Grow up to the maximum size
            entitySize.height = std::min(cellHeight, maxHeight);
        }
        
        // Calculate entity position
        float entityX = columnX[info->gridX];
        float entityY = rowY[info->gridY];
        
        // Apply alignment
        switch (info->constraints->getHorizontalAlign()) {
            case Alignment::Start:
                // Already at start position
                break;
            case Alignment::Center:
                entityX += (cellWidth - entitySize.width) / 2.0f;
                break;
            case Alignment::End:
                entityX += cellWidth - entitySize.width;
                break;
            case Alignment::Fill:
                // Already handled by size adjustment
                break;
            case Alignment::Baseline:
                // Not implemented in basic layout
                break;
        }
        
        switch (info->constraints->getVerticalAlign()) {
            case Alignment::Start:
                // Already at start position
                break;
            case Alignment::Center:
                entityY += (cellHeight - entitySize.height) / 2.0f;
                break;
            case Alignment::End:
                entityY += cellHeight - entitySize.height;
                break;
            case Alignment::Fill:
                // Already handled by size adjustment
                break;
            case Alignment::Baseline:
                // Not implemented in basic layout
                break;
        }
        
        // Apply margins
        entityX += info->constraints->getMargin().left;
        entityY += info->constraints->getMargin().top;
        entitySize.width -= info->constraints->getMargin().left + info->constraints->getMargin().right;
        entitySize.height -= info->constraints->getMargin().top + info->constraints->getMargin().bottom;
        
        // Save for later use
        info->x = entityX;
        info->y = entityY;
        
        // Update entity position and size
        info->entity->setPosition(entityX, entityY);
        info->entity->setSize(entitySize);
    }
}

void FlexGridLayout::applySizeGroups() {
    // For each size group, find the max width and height
    std::unordered_map<std::string, LayoutSize> maxSizes;
    
    for (const auto& pair : sizeGroups_) {
        const std::string& groupName = pair.first;
        const auto& entities = pair.second;
        
        LayoutSize maxSize;
        
        for (auto* entity : entities) {
            auto* info = findEntityInfo(entity);
            if (info && shouldParticipateInLayout(*info)) {
                maxSize.width = std::max(maxSize.width, info->calculatedSize.width);
                maxSize.height = std::max(maxSize.height, info->calculatedSize.height);
            }
        }
        
        maxSizes[groupName] = maxSize;
    }
    
    // Apply max sizes to all entities in each group
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        const std::string& groupName = info.constraints->getSizeGroup();
        if (!groupName.empty()) {
            auto it = maxSizes.find(groupName);
            if (it != maxSizes.end()) {
                info.calculatedSize = it->second;
            }
        }
    }
}

void FlexGridLayout::applyEndGroups() {
    // For each end group, align entities to the same end position
    std::unordered_map<std::string, float> rightEdges;
    std::unordered_map<std::string, float> bottomEdges;
    
    // First pass: find the maximum right and bottom edges for each group
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        const std::string& groupName = info.constraints->getEndGroup();
        if (!groupName.empty()) {
            float rightEdge = info.x + info.calculatedSize.width;
            float bottomEdge = info.y + info.calculatedSize.height;
            
            auto rightIt = rightEdges.find(groupName);
            if (rightIt == rightEdges.end() || rightIt->second < rightEdge) {
                rightEdges[groupName] = rightEdge;
            }
            
            auto bottomIt = bottomEdges.find(groupName);
            if (bottomIt == bottomEdges.end() || bottomIt->second < bottomEdge) {
                bottomEdges[groupName] = bottomEdge;
            }
        }
    }
    
    // Second pass: adjust entities to align with the right/bottom edges
    for (auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        const std::string& groupName = info.constraints->getEndGroup();
        if (!groupName.empty()) {
            auto rightIt = rightEdges.find(groupName);
            if (rightIt != rightEdges.end()) {
                float newX = rightIt->second - info.calculatedSize.width;
                info.entity->setPosition(newX, info.y);
                info.x = newX;
            }
        }
    }
}

bool FlexGridLayout::shouldParticipateInLayout(const EntityInfo& info) const {
    if (!info.entity->isVisible()) {
        HideMode mode = info.constraints->getHideMode() != HideMode::Default ? 
                        info.constraints->getHideMode() : defaultHideMode_;
        return mode == HideMode::Default;
    }
    return true;
}

LayoutConstraints FlexGridLayout::createEntityConstraints(const EntityInfo& info, const LayoutConstraints& containerConstraints) const {
    // Start with container constraints
    LayoutConstraints entityConstraints = containerConstraints;
    
    // Apply minimum size constraints if specified
    if (info.constraints->getMinWidth().getType() != SizeConstraint::CONTENT) {
        entityConstraints.setMinWidth(info.constraints->getMinWidth().calculateSize(containerConstraints.getMaxWidth(), 0));
    }
    
    if (info.constraints->getMinHeight().getType() != SizeConstraint::CONTENT) {
        entityConstraints.setMinHeight(info.constraints->getMinHeight().calculateSize(containerConstraints.getMaxHeight(), 0));
    }
    
    // Apply maximum size constraints if specified
    if (info.constraints->getMaxWidth().getType() != SizeConstraint::CONTENT) {
        float maxWidth = info.constraints->getMaxWidth().calculateSize(containerConstraints.getMaxWidth(), 0);
        // If component is not set to grow, limit max width to preferred width
        if (info.constraints->getGrowX() <= 0.0f) {
            maxWidth = std::min(maxWidth, info.entity->getPreferredSize().width);
        }
        entityConstraints.setMaxWidth(std::min(entityConstraints.getMaxWidth(), maxWidth));
    } else if (info.constraints->getGrowX() <= 0.0f) {
        // If not growing, max width is the preferred width
        entityConstraints.setMaxWidth(std::min(entityConstraints.getMaxWidth(), 
                                             info.entity->getPreferredSize().width));
    }
    
    if (info.constraints->getMaxHeight().getType() != SizeConstraint::CONTENT) {
        float maxHeight = info.constraints->getMaxHeight().calculateSize(containerConstraints.getMaxHeight(), 0);
        // If component is not set to grow, limit max height to preferred height
        if (info.constraints->getGrowY() <= 0.0f) {
            maxHeight = std::min(maxHeight, info.entity->getPreferredSize().height);
        }
        entityConstraints.setMaxHeight(std::min(entityConstraints.getMaxHeight(), maxHeight));
    } else if (info.constraints->getGrowY() <= 0.0f) {
        // If not growing, max height is the preferred height
        entityConstraints.setMaxHeight(std::min(entityConstraints.getMaxHeight(), 
                                              info.entity->getPreferredSize().height));
    }
    
    return entityConstraints;
}

void FlexGridLayout::updateGridDimensions() {
    // Update grid dimensions based on current entity positions
    gridWidth_ = 0;
    gridHeight_ = 0;
    
    for (const auto& info : entities_) {
        if (!shouldParticipateInLayout(info)) {
            continue;
        }
        
        int maxX = info.gridX + info.constraints->getSpanX();
        int maxY = info.gridY + info.constraints->getSpanY();
        
        gridWidth_ = std::max(gridWidth_, maxX);
        gridHeight_ = std::max(gridHeight_, maxY);
    }
}

EntityInfo* FlexGridLayout::findEntityInfo(LayoutEntity* entity) {
    for (auto& info : entities_) {
        if (info.entity == entity) {
            return &info;
        }
    }
    return nullptr;
}

} // namespace LayoutEngine
