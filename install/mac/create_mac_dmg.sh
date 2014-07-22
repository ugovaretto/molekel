# Create image
DMG="$1"_tmp.dmg
hdiutil create -srcfolder ./dist -volname "Molekel" $DMG
# Mount the disk image
hdiutil attach $DMG
# Obtain device information
DEVS=$(hdiutil attach $DMG | cut -f 1)
DEV=$(echo $DEVS | cut -f 1 -d ' ')
# Unmount the disk image
hdiutil detach $DEV
# Convert the disk image to read-only
hdiutil convert $DMG -format UDZO -o "$1".dmg
# Remove temp data
rm $DMG
