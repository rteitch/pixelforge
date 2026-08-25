# PixelForge User Guide

## Open an Image

1. Start PixelForge.
2. Choose **File > Open Image...** or click **Open** on the toolbar.
3. Select a JPEG, PNG, BMP, TIFF, or WebP file.
4. You can also drag a supported image onto the canvas.

To replace the current image, open another file or drop one onto the canvas. If the current project has unsaved changes, PixelForge asks whether to save, discard, or cancel.

To remove the current image, choose **File > Close Image**, click **Close Image** on the toolbar, or press `Ctrl+W`.

## Filters

1. Open an image and keep the **Filters** tab selected.
2. Select a preset from the list or search by name.
3. Adjust intensity, grain, vignette, temperature, tint, contrast, brightness, saturation, highlights, or shadows.
4. The preview updates as controls change.
5. Use **View > Toggle Comparison** or the **Compare** toolbar action to compare the source and result.
6. In comparison mode, drag the white vertical divider left or right to change the Before/After split. Drag elsewhere on the canvas to pan.
7. Use **Save as New Preset** to save the current filter settings.
8. Use **Import Custom LUT (.cube)** to load a compatible LUT and apply it to the current image.

## WPAP

1. Open an image and select the **WPAP** tab.
2. Adjust color count, detail, palette, face detection, and face detail boost.
	- **Vibrant**, **Pastel**, **Mono + Accent**, **Sunset**, and **Ocean** use generated style palettes while assigning colors from each source region.
	- **Auto (K-Means)** builds a palette from the image's dominant colors automatically.
3. PixelForge generates a preview after the controls settle, or immediately when **Generate WPAP** is clicked.
4. Use **Export as SVG** from the WPAP tab after a WPAP result has been generated.
5. Use **File > Export Image...** to export the raster result as PNG, JPEG, or TIFF.

## Crop and Rotate

Choose **Edit > Crop & Rotate...**. Use the rotation, flip, fine rotation, aspect ratio, and crop controls, then choose **Apply**. Choose **Cancel** to discard the dialog changes.

## Undo and Redo

- `Ctrl+Z` or **Edit > Undo** restores the previous editing state.
- `Ctrl+Shift+Z` or **Edit > Redo** restores a state that was undone.
- A new edit after Undo clears the redo path.

## Projects

- **File > Save Project...** saves a `.pforge` project.
- **File > Open Project...** opens a saved project and its referenced source image.
- `Ctrl+Shift+S` opens the Save Project dialog.
- PixelForge asks whether to save unsaved changes before replacing an image or closing the application.

Keep the referenced source image available when moving a `.pforge` file to another computer. Project files store a reference to the source image rather than embedding the image.

## Layers

The **Layers** panel on the right lists applied filters and WPAP results from bottom to top.

- Uncheck a layer to temporarily disable it.
- Select a layer and move the opacity slider to blend it with the result below.
- Use **Up** and **Down** to change the processing order.
- Use **Remove** to delete the selected layer.
- Double-click a layer name to rename it.
- Select a filter or WPAP layer to load its parameters back into the corresponding editing panel.

Layer changes update the preview immediately and are included when the project is saved.

## Batch Processing

1. Choose **File > Batch Process...**.
2. Use **Add Files...** or **Add Folder...** to select input images.
3. Choose an output folder with **Browse...**.
4. Select a filter preset, output format, naming pattern, and JPEG quality. Enable **Apply WPAP instead of filter** to process the batch with WPAP parameters.
5. Choose **Start**. Progress is shown per file and for the complete batch.
6. Choose **Cancel** while processing to request cancellation.
7. Choose **Close** to close the dialog.

Supported naming placeholders are `{name}`, `{preset}`, and `{index}`.

## View and Appearance

- **Zoom In**, **Zoom Out**, **Fit to Window**, and **Actual Size** control the canvas view.
- `Ctrl+=`, `Ctrl+-`, `Ctrl+0`, and `Ctrl+1` are the corresponding shortcuts.
- Mouse-wheel zoom and drag-to-pan are available on the canvas.
- **View > Toggle Dark/Light Theme** changes the application theme.
- **View > Comparison Grid...** previews all built-in filter presets and applies the selected one when clicked.

## Plugins and AI

- **File > Load Plugin...** loads one platform plugin (`.dll` on Windows, `.so` on Linux, `.dylib` on macOS).
- **File > Load Plugin Directory...** loads compatible plugins from a folder.
- **AI Style > Apply AI Style Transfer...** requires an ONNX Runtime build and a compatible `.onnx` model. Without them, PixelForge shows setup guidance instead of applying a style.

## Hardware Diagnostics

Choose **Help > Hardware Diagnostics...** to check whether Windows can identify the display adapter. NVIDIA systems are queried through `nvidia-smi`; other adapters use Windows display-device information when available.

The current build uses CPU/OpenMP for image processing. A detected VGA is reported for diagnostics, but CUDA/OpenCL acceleration is not enabled yet.

## Export and Safety Notes

- **Export Image...** is disabled in practice until an image is open.
- **Export SVG** requires a generated WPAP result.
- Saving a project does not export an image; use **Export Image...** for a final raster file.
- Large images may require significant memory during WPAP, batch, or comparison-grid processing.

## Shortcut Reference

| Shortcut | Action |
|---|---|
| `Ctrl+O` | Open image |
| `Ctrl+W` | Close image |
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `Ctrl+Shift+S` | Save project |
| `Ctrl+Shift+E` | Export image |
| `Ctrl+B` | Batch processing |
| `Ctrl+R` | Crop and rotate |
| `Ctrl+D` | Toggle favorite preset |
| `Ctrl+=` | Zoom in |
| `Ctrl+-` | Zoom out |
| `Ctrl+0` | Fit to window |
| `Ctrl+1` | Actual size |
| `Ctrl+C` | Toggle comparison |
| `Ctrl+G` | Comparison grid |
| `Ctrl+I` | AI style transfer |
| `Ctrl+Q` | Exit |
