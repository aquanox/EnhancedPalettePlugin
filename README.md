[![GitHub release](https://img.shields.io/github/release/aquanox/EnhancedPalettePlugin.svg)](https://github.com/aquanox/EnhancedPalettePlugin/releases)
[![GitHub license](https://img.shields.io/github/license/aquanox/EnhancedPalettePlugin)](https://github.com/aquanox/EnhancedPalettePlugin/blob/main/LICENSE)
[![GitHub forks](https://img.shields.io/github/forks/aquanox/EnhancedPalettePlugin)](https://github.com/aquanox/EnhancedPalettePlugin/network)
[![GitHub stars](https://img.shields.io/github/stars/aquanox/EnhancedPalettePlugin)](https://github.com/aquanox/EnhancedPalettePlugin/stargazers)
![UE5](https://img.shields.io/badge/UE5-5.4+-lightgrey)

## Enhanced Palette Plugin for Unreal Engine

> [!CAUTION]
> This plugin is currently Experimental, while most of features are functional there is room for improvements.

This plugin makes `Place Actors` panel configurable from Editor Settings 
and provides additional customization features for the category content.

Primary plugin usage is aimed for completing following use cases:
* Having custom category for Project-Specific items used globally across project.
* Having custom dynamic category for Project-Specific items gathered from Project files (for example selecting them by AssetDataTag, Class etc).
* Having custom category for Level-Specific items, changing dynamically for based on currently edited Level or already present actors.
* Other cases where having custom list of placeable items would be useful.
* Hiding not useful/needed standard engine categories.

The plugin provides three "types" of defining custom palette categories: Static, Dynamic and External.

### Static Category

Entirely defined in settings panel with list of placeable items.

![](Images/EPP-Config-Static.png)

### Dynamic Category

A Class or Blueprint containing category information and implementing item selection logic for the category content.

Dynamic category can update it's content dynamically in runtime based on user triggers.

![](Images/EPP-Config-AssetReg.png)

### External Category (Experimental)

Subsystem exposes functions to register categories in runtime from editor utility blueprints.

This mode is an experiment.

## Unreal Engine Versions

Plugin relies on Instanced Structs, compiles and tested on UnrealEngine 5.4 and 5.5.
For other engine versions some slight patching would be needed.

## License

Enhanced Palette Plugin is available under the MIT license. See the LICENSE file for more info.

## Image Previews

![](Images/EPP-Preview-Categories.png)

![](Images/EPP-Preview-DynamicCategory.png)
