//
// layoutVisualization.hpp
// Layout Visualization Utilities for LuaXmlWidgets
//

#ifndef layoutVisualization_hpp
#define layoutVisualization_hpp

#include "layoutEngine.hpp"
#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/image.h>
#include <vector>

namespace LayoutVisualization {

/**
 * Visualizes a collection of layout entities and saves as a bitmap image
 * @param entities Vector of LayoutEntity pointers to visualize
 * @param containerWidth Width of the container in pixels
 * @param containerHeight Height of the container in pixels
 * @param filename Output filename (default: /tmp/layout_entities.png)
 * @return true if successful, false otherwise
 */
bool visualizeEntities(const std::vector<LayoutEngine::LayoutEntity*>& entities,
                      int containerWidth, int containerHeight,
                      const wxString& filename = wxT("/tmp/layout_entities.png"));

/**
 * Visualizes a FlexGridLayout and saves as a bitmap image
 * This function extracts entities from the layout and visualizes them
 * @param layout Pointer to the FlexGridLayout to visualize
 * @param containerWidth Width of the container in pixels
 * @param containerHeight Height of the container in pixels
 * @param filename Output filename (default: /tmp/layout_visualization.png)
 * @return true if successful, false otherwise
 */
bool visualizeLayout(LayoutEngine::FlexGridLayout* layout,
                    int containerWidth, int containerHeight,
                    const wxString& filename = wxT("/tmp/layout_visualization.png"));

/**
 * Creates a demonstration layout with various widgets and visualizes it
 * This is useful for testing the visualization system
 * @param filename Output filename (default: /tmp/demo_layout.png)
 * @return true if successful, false otherwise
 */
bool createDemoVisualization(const wxString& filename = wxT("/tmp/demo_layout.png"));

/**
 * Configuration options for visualization
 */
struct VisualizationConfig {
    // Colors
    wxColour backgroundColor = wxColour(255, 255, 255);      // White
    wxColour containerColor = wxColour(240, 240, 240);       // Light gray
    wxColour borderColor = wxColour(0, 0, 0);                // Black
    wxColour textColor = wxColour(0, 0, 0);                  // Black
    
    // Fonts
    int titleFontSize = 16;
    int labelFontSize = 10;
    int dimFontSize = 8;
    int coordFontSize = 7;
    
    // Layout
    int padding = 25;
    int titleHeight = 50;
    bool showDimensions = true;
    bool showCoordinates = true;
    bool showEntityNames = true;
    
    // Entity colors (will cycle through these)
    std::vector<wxColour> entityColors = {
        wxColour(135, 206, 235), // Sky blue
        wxColour(255, 182, 193), // Light pink
        wxColour(152, 251, 152), // Pale green
        wxColour(255, 218, 185), // Peach
        wxColour(221, 160, 221), // Plum
        wxColour(255, 255, 224), // Light yellow
        wxColour(175, 238, 238), // Pale turquoise
        wxColour(255, 228, 196)  // Bisque
    };
};

/**
 * Advanced visualization function with configuration options
 * @param entities Vector of LayoutEntity pointers to visualize
 * @param containerWidth Width of the container in pixels
 * @param containerHeight Height of the container in pixels
 * @param config Visualization configuration
 * @param filename Output filename
 * @return true if successful, false otherwise
 */
bool visualizeEntitiesAdvanced(const std::vector<LayoutEngine::LayoutEntity*>& entities,
                              int containerWidth, int containerHeight,
                              const VisualizationConfig& config,
                              const wxString& filename);

} // namespace LayoutVisualization

#endif /* layoutVisualization_hpp */
