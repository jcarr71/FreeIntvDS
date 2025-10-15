#!/usr/bin/env python3
"""
Create a simple test PNG for FreeIntvDS overlay testing
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Create a 256x224 image (overlay dimensions)
width, height = 256, 224
image = Image.new('RGB', (width, height), color='black')
draw = ImageDraw.Draw(image)

# Draw a clear pattern that will be obvious if it's working
# Background: Dark blue
image = Image.new('RGB', (width, height), color=(0, 0, 100))
draw = ImageDraw.Draw(image)

# Draw colored rectangles in corners
# Top-left: Red
draw.rectangle([0, 0, 64, 56], fill=(255, 0, 0))

# Top-right: Green  
draw.rectangle([192, 0, 256, 56], fill=(0, 255, 0))

# Bottom-left: Blue
draw.rectangle([0, 168, 64, 224], fill=(0, 0, 255))

# Bottom-right: Yellow
draw.rectangle([192, 168, 256, 224], fill=(255, 255, 0))

# Center: White rectangle with text
draw.rectangle([64, 80, 192, 144], fill=(255, 255, 255))

# Try to add text (will work if font is available)
try:
    # Use default font
    font = ImageFont.load_default()
    text = "OVERLAY\nTEST"
    
    # Get text size and center it
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    
    x = (width - text_width) // 2
    y = (height - text_height) // 2
    
    draw.text((x, y), text, fill=(0, 0, 0), font=font)
except:
    # If font fails, just draw a simple pattern
    draw.line([64, 112, 192, 112], fill=(0, 0, 0), width=3)
    draw.line([128, 80, 128, 144], fill=(0, 0, 0), width=3)

# Draw border around entire image
draw.rectangle([0, 0, width-1, height-1], outline=(255, 255, 255), width=2)

# Save the test PNG
output_path = 'test_overlay.png'
image.save(output_path, 'PNG')

print(f"Test PNG created: {output_path}")
print(f"Dimensions: {width}x{height}")
print("Pattern: Red/Green corners at top, Blue/Yellow at bottom, White center with border")
print("\nTo use this test:")
print("1. Copy test_overlay.png to your RetroArch overlays directory")
print("2. Rename it to match your ROM name (e.g., 'Astrosmash (USA, Europe).png')")
print("3. Load the game in RetroArch")
print("4. You should see colored rectangles instead of static")