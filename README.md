# Custom Asset Management System for Unreal Engine 5.4

This project demonstrates a custom asset management system for Unreal Engine 5.4, implemented primarily in C++. The system allows for efficient loading, unloading, and tracking of custom asset types, with support for exporting asset data to CSV/Excel.
This is a test project where I am using my own Asset Manager as base to enhance it with extra logic by the use of Cursor.


## Features

- Custom Asset Manager that extends UE's AssetManager
- Base asset class with common properties (ID, name, description, tags, etc.)
- Asset table for storing and managing assets in a tabular format
- CSV import/export functionality
- Unique identifiers for all assets
- Sample custom asset type (Item)
- Level-specific asset loading
- Custom asset editor window
- **Asset Prefetching System**: Proactively loads assets based on player position
- **Asset Compression Tiers**: Control memory vs. load speed with four compression levels
- **Level Streaming Integration**: Automatically load/unload bundles with level streaming
- **Asset Hotswapping**: Replace assets at runtime without restarting the game

## Designer's Guide: Using the Custom Asset System

This guide is for designers and content creators who need to work with the Custom Asset System without dealing with code.

### Getting Started

1. **Open the Asset Manager Window**:
   - In the editor, click on the "Window" menu in the top bar
   - Select "Custom Asset Manager"
   - The Asset Manager window provides tabs for managing Assets, Bundles, Memory Usage, and Dependencies

2. **Working with Asset Tabs**:
   - **Assets Tab**: View and manage all custom assets
   - **Bundles Tab**: Organize assets into logical groups
   - **Memory Tab**: Monitor asset memory usage
   - **Dependencies Tab**: See relationships between assets

### Creating and Managing Assets

1. **Creating a New Asset**:
   - Right-click in the Content Browser
   - Select "Miscellaneous" → "Data Asset"
   - Choose an asset type (e.g., "Custom Item Asset" or "Custom Character Asset")
   - Name your asset and save it

2. **Setting Up an Asset**:
   - Double-click the asset to open its editor
   - **Important fields to set**:
     - **Asset ID**: A unique identifier (e.g., "health_potion_01")
     - **Display Name**: User-friendly name (e.g., "Health Potion")
     - **Description**: Detailed information about the asset
     - **Tags**: Keywords to help categorize and search (e.g., "Potion, Health, Consumable")

3. **Using the Item Asset Template**:
   - **Value**: In-game value/price
   - **Weight**: How much it weighs in the inventory
   - **Quality**: Set rarity (Common, Uncommon, Rare, Epic, Legendary)
   - **Category**: Type of item (Weapon, Armor, Consumable, etc.)
   - **Stackable**: Whether multiple items can stack in inventory
   - **Max Stack Size**: Maximum number in a stack (if stackable)
   - **Consumable**: Whether the item can be used/consumed
   - **Effects**: What happens when the item is used

4. **Using the Character Asset Template**:
   - **Character Class**: Type of character (Warrior, Mage, Rogue, etc.)
   - **Base Health/Movement**: Starting stats
   - **Visual Assets**: Character mesh, animation blueprint, etc.
   - **Abilities**: Powers the character can use
   - **Attributes**: Strength, Dexterity, Intelligence, etc.
   - **Level/Experience**: Progression settings

### Working with Asset Bundles

1. **Creating an Asset Bundle**:
   - In the Asset Manager window, go to the "Bundles" tab
   - Click "Create New Bundle"
   - Set a unique Bundle ID and Display Name

2. **Adding Assets to a Bundle**:
   - Select a bundle in the list
   - Click "Add Assets"
   - Select assets from the picker dialog
   - Click "Add Selected"

3. **Bundle Settings**:
   - **Priority**: Higher priority bundles load first
   - **Preload At Startup**: Bundle loads when the game starts
   - **Keep In Memory**: Bundle stays loaded even when not in use

### Using Advanced Asset Features

1. **Asset Prefetching System**:
   - Automatically loads assets before they're needed based on player position
   - **Setup**:
     - In the Asset Manager window, go to the "Advanced" tab
     - Enable "Spatial Asset Prefetching"
     - Set the "Prefetch Radius" (how far ahead to load assets)
     - Use "RegisterAssetLocation" in Blueprints to assign world positions to assets
   - **Benefits**: Smoother gameplay with fewer loading hitches

2. **Asset Compression Tiers**:
   - Control the balance between memory usage and loading speed
   - **Compression Levels**:
     - **None**: Uncompressed for fastest loading (largest file size)
     - **Low**: Minimal compression, good performance
     - **Medium**: Balanced compression (default)
     - **High**: Maximum compression, slower loading but smallest size
   - **Setting Compression**:
     - Select an asset in the "Assets" tab
     - Choose a compression tier from the dropdown
     - Apply changes with "Recompress Asset"
   - **Best Practices**:
     - Use low/no compression for frequently used assets
     - Use high compression for rarely used assets

3. **Level Streaming Integration**:
   - Automatically load and unload asset bundles with level streaming
   - **Setup**:
     - Select a bundle in the "Bundles" tab
     - Click "Edit Level Associations"
     - Add level names and set:
       - Preload Distance: How close the player needs to be
       - Unload with Level: Whether to unload when the level unloads
   - **Runtime Behavior**:
     - Assets automatically load when the player approaches a level
     - Assets unload when the level unloads (if configured)
   - **Performance Monitoring**:
     - Use the "Memory" tab to see which levels are consuming resources

4. **Asset Hotswapping**:
   - Replace assets during runtime without restarting the game
   - **Usage**:
     - In the "Assets" tab, select an asset
     - Click "Create New Version"
     - Modify the asset properties
     - Click "Queue Hotswap"
     - Apply pending hotswaps with "Apply Hotswaps" button
   - **Development Benefits**:
     - Test asset changes without restarting the game
     - Update live games with new content
   - **Listening for Changes**:
     - Set up Blueprint event listeners to respond to asset changes

### Level-Specific Asset Loading

1. **Setting Up Level-Specific Assets**:
   - Create a new bundle for each level (e.g., "level_forest", "level_dungeon")
   - Add all assets needed specifically for that level to its bundle
   - Configure the bundle settings:
     - Set "Auto Load On Level" to the appropriate level name
     - Disable "Preload At Startup" unless needed for the first level

2. **Implementing Level Asset Rules**:
   - Open the level in the Unreal Editor
   - In the World Settings panel:
     - Find the "Custom Asset Management" section
     - Add Bundle IDs that should be loaded when this level is active
     - Set priorities for each bundle
     - Configure unloading behavior (e.g., "Unload On Level Exit")

3. **Testing Level-Specific Loading**:
   - Enable "Show Debug Info" in the Asset Manager window
   - Play the game in editor
   - Use the console command `ShowAssetLoading 1` to see real-time asset loading/unloading
   - Watch the Memory tab to verify assets load and unload correctly

4. **Advanced Level Asset Management**:
   - Create "transition" bundles for assets needed during level streaming
   - Set up "persistent" bundles for assets used across multiple levels
   - Use "sub-area" bundles for sections of large levels to manage memory better

### Working with Asset Data Tables

1. **What Are Asset Data Tables**:
   - Data tables provide a spreadsheet-like way to define multiple assets at once
   - They're especially useful for items, enemies, or other assets with similar structures
   - Tables can be edited in Unreal Editor or exported/imported via CSV

2. **Creating a New Asset Data Table**:
   - Right-click in the Content Browser
   - Select "Miscellaneous" → "Data Table"
   - Choose a row structure (e.g., "CustomItemRow" or "CustomCharacterRow")
   - Name your table and save it

3. **Editing Asset Tables in Unreal Editor**:
   - Double-click the table to open the Data Table Editor
   - Click "+" to add a new row
   - Give each row a unique name (this becomes part of the Asset ID)
   - Fill in the values for each column
   - Click "Save" when finished

4. **Importing from CSV/Excel**:
   - Create a CSV file with the correct column headers (matching property names)
   - First column must be "Name" (row identifier)
   - In the Data Table Editor, click "Import"
   - Select your CSV file
   - Review any errors or warnings

5. **Exporting to CSV/Excel**:
   - In the Data Table Editor, click "Export"
   - Choose a save location for the CSV file
   - Open in Excel or another spreadsheet program for bulk editing

6. **Using Data Tables in Game**:
   - Data tables are automatically registered with the Asset Manager
   - Reference table rows using the pattern `TableName.RowName`
   - Access individual properties using Blueprint nodes or C++ code

7. **Example: Item Table Setup**:
   - Create a Data Table with the "CustomItemRow" structure
   - Add rows for different items (e.g., "HealthPotion", "IronSword")
   - Set common properties (Category, Weight, Value) for each item
   - This allows creating dozens or hundreds of items quickly

### Best Practices for Designers

1. **Use Consistent Naming**:
   - For Asset IDs: `category_specific_variant` (e.g., `sword_iron_01`)
   - Keep names lowercase with underscores for better compatibility

2. **Organize with Tags**:
   - Add relevant tags to make finding assets easier
   - Use consistent tag terminology across the team

3. **Asset Relationships**:
   - Use the Dependencies tab to see what assets rely on others
   - Be careful when modifying assets that have many dependents

4. **Memory Management**:
   - Check the Memory tab to see which assets use the most resources
   - Consider using LOD (Level of Detail) for large assets
   - Group assets into logical bundles for efficient loading/unloading

5. **Testing Changes**:
   - Use the "Refresh" button in the toolbar to update the asset displays
   - Test assets in play mode to verify they work as expected

6. **Data Table Best Practices**:
   - Keep a backup of your CSV files
   - Document the meaning of each column for your team
   - Use data validation formulas in Excel to catch errors
   - Consider splitting very large tables into smaller, more focused ones

### Troubleshooting Common Issues

1. **Asset Not Appearing In-Game**:
   - Check that the Asset ID is unique and properly set
   - Verify the asset is part of a bundle that's being loaded
   - Use `ShowAssetLoading 1` console command to debug

2. **Changes Not Showing Up**:
   - Click the "Refresh" button in the Asset Manager window
   - Try closing and reopening the Asset Manager window

3. **Import/Export Problems**:
   - Ensure CSV files use correct formatting
   - Don't change the column headers or Asset IDs

4. **Performance Issues**:
   - Check the Memory tab for assets using excessive resources
   - Consider adjusting LOD settings or texture sizes
   - Review your bundle loading strategy to avoid loading unnecessary assets

5. **Level-Specific Assets Not Loading**:
   - Verify the bundle is correctly associated with the level
   - Check log messages for loading errors
   - Make sure asset references are valid

6. **Prefetching Not Working**:
   - Verify assets have registered locations via `RegisterAssetLocation`
   - Check that prefetch radius is appropriate for your game's scale
   - Ensure prefetching is enabled in the advanced settings

7. **Compression Tier Issues**:
   - Remember that recompression is only available in editor builds
   - Some assets may not compress well (especially already-compressed textures)
   - High compression increases load time, so balance appropriately

8. **Level Streaming Integration Problems**:
   - Ensure level names match exactly (case-sensitive)
   - Set reasonable preload distances based on level size
   - Verify streaming levels are properly set up in the World Settings

9. **Hotswapping Failures**:
   - Ensure both versions have the same Asset ID
   - Listeners must implement the correct interface function
   - Some complex assets may not hotswap cleanly and require a full reload

For more technical details or custom implementations, please consult with the programming team.

## Architecture

- `UCustomAssetBase`: Base class for all custom assets
- `UCustomAssetManager`: Manages loading, unloading, and tracking assets
- `UCustomAssetTable`: Data table for storing asset information
- `UCustomItemAsset`: Sample asset type for items
- `UCustomCharacterAsset`: Asset type for character data
- `FCustomAssetEditorModule`: Editor module for the Custom Asset Manager
- `UCustomAssetMemoryTracker`: Tracks memory usage of loaded assets
- `UCustomAssetBundle`: Groups related assets for efficient loading/unloading
- `FAssetHotswapInfo`: Structure for asset version information during hotswaps
- `FBundleLevelAssociation`: Links asset bundles to specific levels
- `EAssetCompressionTier`: Enum defining compression levels for assets

## License

This project is licensed under the MIT License - see the LICENSE file for details. 