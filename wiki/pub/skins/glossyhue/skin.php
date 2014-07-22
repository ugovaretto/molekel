<?php if (!defined('PmWiki')) exit();
/* PmWiki GlossyHue skin
 *
 * Examples at: http://pmwiki.com/Cookbook/GlossyHue and http://solidgone.com/Skins/
 * Based on Glossy Blue by Nick La http://www.ndesign-studio.com/
 * Copyright (c) 2009 David Gilbert
 * Dual licensed under the MIT and GPL licenses:
 *    http://www.opensource.org/licenses/mit-license.php
 *    http://www.gnu.org/licenses/gpl.html
 */
global $FmtPV;
$FmtPV['$SkinName'] = '"GlossyHue"';
$FmtPV['$SkinVersion'] = '"1.2.0"';

# Create a nosearch markup, since one doesn't exist
Markup('nosearch', 'directives',  '/\\(:nosearch:\\)/ei', "SetTmplDisplay('PageSearchFmt',0)");

# ----------------------------------------
# - Standard Skin Setup
# ----------------------------------------
$FmtPV['$WikiTitle'] = '$GLOBALS["WikiTitle"]';

# Define a link stye for new page links
global $LinkPageCreateFmt;
SDV($LinkPageCreateFmt, "<a class='createlinktext' href='\$PageUrl?action=edit'>\$LinkText</a>");

# Default color scheme
global $SkinColor, $ValidSkinColors;
if ( !is_array($ValidSkinColors) ) $ValidSkinColors = array();
array_push($ValidSkinColors, 'blue');
if ( isset($_GET['color']) && in_array($_GET['color'], $ValidSkinColors) ) {
	$SkinColor = $_GET['color'];
} elseif ( !in_array($SkinColor, $ValidSkinColors) ) {
	$SkinColor = 'blue';
}

# Override pmwiki styles otherwise they will override styles declared in css
global $HTMLStylesFmt;
$HTMLStylesFmt['pmwiki'] = '';

# Add a custom page storage location
global $WikiLibDirs;
$PageStorePath = dirname(__FILE__)."/wikilib.d/{\$FullName}";
$where = count($WikiLibDirs);
if ($where>1) $where--;
array_splice($WikiLibDirs, $where, 0, array(new PageStore($PageStorePath)));
