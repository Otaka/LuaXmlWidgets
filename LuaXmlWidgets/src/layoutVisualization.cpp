//
// layoutVisualization.cpp
// Layout Visualization Utilities Implementation
//

#include "layoutVisualization.hpp"
#include <iostream>

namespace LayoutVisualization {

bool visualizeEntities(const std::vector<LayoutEngine::LayoutEntity*>& entities,
                      int containerWidth, int containerHeight,
                      const wxString& filename) {
    VisualizationConfig config; // Use default configuration
    return visualizeEntitiesAdvanced(entities, containerWidth, containerHeight, config, filename);
}

bool visualizeLayout(LayoutEngine::FlexGridLayout* layout,
                    int containerWidth, int containerHeight,
                    const wxString& filename) {
    if (!layout) {
        wxLogError(wxT("Layout pointer is null"));
        return false;
    }
    
    // Get entities from the layout
    std::vector<LayoutEngine::LayoutEntity*> entities = layout->getEntityList();
    
    // Visualize the entities
    bool result = visualizeEntities(entities, containerWidth, containerHeight, filename);
    
    if (result) {
        // Also print layout debug info to console
        std::string debugInfo = layout->getLayoutDebugInfo(true, true);
        std::cout << "=== Layout Debug Information ===" << std::endl;
        std::cout << debugInfo << std::endl;
        std::cout << "=================================" << std::endl;
    }
    
    return result;
}

bool createDemoVisualization(const wxString& filename) {
    // Create a complex demo layout
    LayoutEngine::FlexGridLayout* layout = LayoutEngine::parseLayoutConstraints("wrap 3, gap 10, insets 15");
    
    // Create various widgets
    LayoutEngine::LayoutEntity titleLabel(280, 30);
    titleLabel.setName("Application Title");
    
    LayoutEngine::LayoutEntity nameLabel(80, 25);
    nameLabel.setName("Name:");
    
    LayoutEngine::LayoutEntity nameField(150, 25);
    nameField.setName("NameInput");
    
    LayoutEngine::LayoutEntity emailLabel(80, 25);
    emailLabel.setName("Email:");
    
    LayoutEngine::LayoutEntity emailField(150, 25);
    emailField.setName("EmailInput");
    
    LayoutEngine::LayoutEntity submitButton(100, 30);
    submitButton.setName("Submit");
    
    LayoutEngine::LayoutEntity resetButton(100, 30);
    resetButton.setName("Reset");
    
    LayoutEngine::LayoutEntity statusArea(280, 60);
    statusArea.setName("Status Area");
    
    // Create constraints
    LayoutEngine::EntityConstraints* titleConstraints = LayoutEngine::parseEntityConstraints("span 3, alignx center");
    LayoutEngine::EntityConstraints* labelConstraints = LayoutEngine::parseEntityConstraints("alignx right");
    LayoutEngine::EntityConstraints* fieldConstraints = LayoutEngine::parseEntityConstraints("span 2, alignx fill, growx");
    LayoutEngine::EntityConstraints* buttonConstraints = LayoutEngine::parseEntityConstraints("sizegroup buttons");
    LayoutEngine::EntityConstraints* statusConstraints = LayoutEngine::parseEntityConstraints("span 3, alignx fill, growy");
    
    // Add widgets to layout
    layout->addEntity(&titleLabel, titleConstraints);
    layout->addEntity(&nameLabel, labelConstraints);
    layout->addEntity(&nameField, fieldConstraints);
    layout->addEntity(&emailLabel, labelConstraints);
    layout->addEntity(&emailField, fieldConstraints);
    layout->addEntity(&submitButton, buttonConstraints);
    layout->addEntity(&resetButton, buttonConstraints);
    layout->addEntity(&statusArea, statusConstraints);
    
    // Perform layout
    LayoutEngine::LayoutConstraints container;
    container.setMaxWidthHeight(400, 300);
    LayoutEngine::LayoutSize totalSize = layout->performLayout(container);
    
    // Generate visualization
    bool result = visualizeLayout(layout, 400, 300, filename);
    
    if (result) {
        std::cout << "Demo layout created with total size: " << totalSize.width << "x" << totalSize.height << std::endl;
        std::cout << "Visualization saved to " << filename.ToStdString() << std::endl;
    }
    
    delete layout;
    return result;
}

bool visualizeEntitiesAdvanced(const std::vector<LayoutEngine::LayoutEntity*>& entities,
                              int containerWidth, int containerHeight,
                              const VisualizationConfig& config,
                              const wxString& filename) {
    
    // Calculate total bitmap size
    int totalWidth = containerWidth + (config.padding * 2);
    int totalHeight = containerHeight + config.titleHeight + (config.padding * 2);
    
    // Create a memory DC with the specified dimensions
    wxBitmap bitmap(totalWidth, totalHeight);
    wxMemoryDC memDC(bitmap);
    
    // Create graphics context for anti-aliased drawing
    wxGraphicsContext* gc = wxGraphicsContext::Create(memDC);
    if (!gc) {
        wxLogError(wxT("Failed to create graphics context"));
        return false;
    }
    
    // Clear background
    gc->SetBrush(wxBrush(config.backgroundColor));
    gc->DrawRectangle(0, 0, totalWidth, totalHeight);
    
    // Draw container border
    gc->SetPen(wxPen(config.borderColor, 2));
    gc->SetBrush(wxBrush(config.containerColor));
    gc->DrawRectangle(config.padding, config.titleHeight, containerWidth, containerHeight);
    
    // Draw title
    if (config.showEntityNames) {
        gc->SetFont(wxFont(config.titleFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        gc->SetBrush(wxBrush(config.textColor));
        gc->DrawText(wxT("Layout Entity Visualization"), config.padding, 10);
    }
    
    // Draw layout information
    gc->SetFont(wxFont(config.labelFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    wxString infoText = wxString::Format(wxT("Container: %dx%d, Entities: %zu"), 
                                       containerWidth, containerHeight, entities.size());
    gc->DrawText(infoText, config.padding, 30);
    
    // Draw each entity
    for (size_t i = 0; i < entities.size(); ++i) {
        const LayoutEngine::LayoutEntity* entity = entities[i];
        if (!entity || !entity->isVisible()) continue;
        
        // Get entity position and size
        float x = entity->getX() + config.padding;
        float y = entity->getY() + config.titleHeight;
        LayoutEngine::LayoutSize size = entity->getSize();
        
        // Choose color
        wxColour color = config.entityColors[i % config.entityColors.size()];
        
        // Draw entity rectangle
        gc->SetPen(wxPen(config.borderColor, 1));
        gc->SetBrush(wxBrush(color));
        gc->DrawRectangle(x, y, size.width, size.height);
        
        // Draw entity label
        if (config.showEntityNames) {
            gc->SetFont(wxFont(config.labelFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
            gc->SetBrush(wxBrush(config.textColor));
            
            wxString label;
            if (!entity->getName().empty()) {
                label = wxString::FromUTF8(entity->getName().c_str());
            } else {
                label = wxString::Format(wxT("Entity%zu"), i + 1);
            }
            
            // Draw label inside entity if there's space, otherwise above it
            if (size.width > 60 && size.height > 20) {
                gc->DrawText(label, x + 5, y + 5);
            } else if (y > config.titleHeight + 15) {
                gc->DrawText(label, x, y - 15);
            }
        }
        
        // Draw dimensions text
        if (config.showDimensions && size.width > 40 && size.height > 30) {
            wxString dimText = wxString::Format(wxT("%.0fx%.0f"), size.width, size.height);
            gc->SetFont(wxFont(config.dimFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            gc->DrawText(dimText, x + 5, y + size.height - 15);
        }
        
        // Draw position coordinates
        if (config.showCoordinates && size.width > 60 && size.height > 45) {
            wxString posText = wxString::Format(wxT("(%.0f,%.0f)"), entity->getX(), entity->getY());
            gc->SetFont(wxFont(config.coordFontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
            gc->DrawText(posText, x + 5, y + 18);
        }
    }
    
    delete gc;
    
    // Save the bitmap as PNG
    wxImage image = bitmap.ConvertToImage();
    if (!image.SaveFile(filename, wxBITMAP_TYPE_PNG)) {
        wxLogError(wxT("Failed to save layout visualization to %s"), filename.c_str());
        return false;
    } else {
        wxLogMessage(wxT("Layout visualization saved to %s"), filename.c_str());
        std::cout << "Layout visualization saved to " << filename.ToStdString() << std::endl;
        return true;
    }
}

} // namespace LayoutVisualization
