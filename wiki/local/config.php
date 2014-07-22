<?php if (!defined('PmWiki')) exit();
$WikiTitle = 'Molekel';
//$ScriptUrl = 'http://www.bioinformatics.org/molekel/wiki';
//$PubDirUrl = 'http://www.bioinformatics.org/molekel/wiki/pub';
$PASSWD=crypt('cscsmolekel');
$DefaultPasswords['admin'] = $PASSWD;
//$DefaultPasswords['read'] = crypt('uvaretto@cscs.ch');
$DefaultPasswords['edit'] = $PASSWD;
$DefaultPasswords['attr'] = $PASSWD;
$DefaultPasswords['upload'] = $PASSWD;
$Skin = "molekel_glossyhue";
$SkinColor = "turquoise";
$ValidSkinColors = array("blue","turquoise");
$EnableGUIButtons = 1;                  // Buttons in the HTML editor
$LinkWikiWords = 0;                     // Automatic links for MixedCase words
$EnablePathInfo = 1;                    // Short URLs
$EnableUpload = 1;
$UploadMaxSize = 10000001;
#include_once("cookbook/VisitorsLogging.php");
include_once("cookbook/mailform.php");
$MailFormAddresses['MOLEKEL']='molekel@cscs.ch';
$MailFormDefaultSender = 'molekel@bioinformatics.org';
//include_once("cookbook/swf.php");
//include_once("cookbook/flash.php");
$UploadPrefixFmt='/$Group/$Name';

$FmtPV['$ResDir']="'" . substr($ScriptUrl,0,strlen($ScriptUrl)-strlen('/pmwiki.php')) . "/uploaded'";
include_once("$FarmD/cookbook/mini.php");
$Mini['EnableLightbox'] = 1;

include_once("$FarmD/cookbook/youtube2.php");
include_once("$FarmD/cookbook/bliptv.php");
include_once("$FarmD/cookbook/slideshare.php");

