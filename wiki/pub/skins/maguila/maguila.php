<?php if (!defined('PmWiki')) exit();

global	$action,$Charset,$WikiLibDirs,$FmtPV,$HTMLStylesFmt,
	$Now,$fnt,$col;

## Skin id

global	$RecipeInfo,$SkinName,$SkinRecipeName;

$RecipeInfo['MaguilaSkin']['Version'] = '2007-11-05';
$SkinName = 'maguila';
$SkinRecipeName = "MaguilaSkin";

## skin page variables

global	$SkinVersionDate,$SkinVersionNum,$SkinVersion,
	$SkinRecipeName,$SkinSourceURL;

$SkinVersionDate = $RecipeInfo['MaguilaSkin']['Version'];
$SkinVersionNum = str_replace("-","",$SkinVersionDate);
$SkinVersion = $SkinName." ".$SkinVersionDate;
$SkinSourceUrl = 'http://www.pmwiki.org/wiki/Cookbook/'.$SkinRecipeName;

## setting variables as page variables

$FmtPV['$SkinName'] = '$GLOBALS["SkinName"]';
$FmtPV['$SkinVersionDate'] = '$GLOBALS["SkinVersionDate"]';
$FmtPV['$SkinVersionNum'] = '$GLOBALS["SkinVersionNum"]';
$FmtPV['$SkinVersion'] = '$GLOBALS["SkinVersion"]';
$FmtPV['$SkinRecipeName'] = '$GLOBALS["SkinRecipeName"]';
$FmtPV['$SkinSourceUrl'] = 'PUE($GLOBALS["SkinSourceUrl"])';

## options

global	$EnableSkinOptions,$EnableNoSections,
	$EnableFontOptions,$EnableActionHighlight;

SDV($EnableSkinOptions, '1');

SDV($EnableSkinTitle, '1');

SDV($EnableNoSections, '1');

SDV($EnableFontOptions, '1');

SDV($EnableActionHighlight, '1');

SDV($SkinCookieExpiration,$Now+60*60*24*365);

## Wiki Skin Title -  function from beeblebrox net gila

global $WikiSkinTitle, $PageLogoSpacer, $WikiTitle, $AsSpacedFunction;

SDV($SkinTitleSpacer, '');

if($EnableSkinOptions && $EnableSkinTitle){
	$spaced_title = preg_split('/[\\s]+/', $AsSpacedFunction($WikiTitle));
	for ($title_index = 0; $title_index < count($spaced_title); $title_index++) {
		if($title_index % 2 == 0) {
		$html_title .= $spaced_title[$title_index] . $SkinTitleSpacer;
		} else {
		$html_title .= "<span>" . $spaced_title[$title_index] . "</span>" . $SkinTitleSpacer;
		}
	}
	SDV($WikiSkinTitle,"<a href='$ScriptUrl?action=home'>$html_title</a>");
}else{
	$WikiSkinTitle = $WikiTitle;
}

## WikiSubTitle

global	$WikiSubTitle;

SDV($WikiSubTitle, "My Gila");

##Some formats

$FmtPV['$Today'] = 'strftime("%a, %d %b %y .", time() )';
$FmtPV['$LastModifiedDate'] = 'strftime("%a, %d %b %y", $page["time"])';
$FmtPV['$Lang']= '$GLOBALS["XLLangs"][0]';

## Side bar vars
global $NoSideBars, $NoLeftBar, $NoRightBar;

SDV($NoSideBars , " div#grid-main,div#grid-wrap{
	width:100%;margin:0;padding:0;float:none;border:0}
 div#grid-left,div#grid-right{
	margin:0;width:0;float:none}\n");

SDV($NoRightBar, " div#grid-main{
	padding-right:0;border-right:0;}
 div#grid-right{
	width:0;margin-right:0;float:none;border:0}
 div#grid-wrap{
	padding-right:0}\n");

SDV($NoLeftBar, " div#grid-main{
	padding-left:0;border-left:0}
 div#grid-left{
	width:0;margin-left:0;float:none;border:0}
 div#grid-wrap{
	padding-left:0}\n");

## page highlight inside side bars

$PageHighLight = "  div#wiki-left ul li a.selflink,div#wiki-right ul li a.selflink{
	background:white;}
  div#wiki-left p.sidehead a.selflink,div#wiki-right p.sidehead a.selflink{
	background:white;}\n";

## Highlight for actions

$ActionHi['Edit']   = "  div#wiki-action li.edit a{
	border-bottom:1px solid white;} 
  div#wiki-foot li.edit a{
	border-bottom:2px solid black}";
$ActionHi['Browse'] = "  div#wiki-action li.browse a{
	border-bottom:1px solid white}
  div#wiki-foot li.browse a{
	border-bottom:2px solid black}";
$ActionHi['Diff']   = "  div#wiki-action li.diff a{
	border-bottom:1px solid white}
  div#wiki-foot li.diff a{
	border-bottom:2px solid black}";
$ActionHi['LogIn']  = "  div#wiki-action li.login a{
	border-bottom:1px solid white}
  div#wiki-foot li.login a{
	border-bottom:2px solid black}";
$ActionHi['LogOut'] = "  div#wiki-action li.logout a{
	border-bottom:1px solid white}
  div#wiki-foot li.logout a{
	border-bottom:2px solid black}";
$ActionHi['Search'] = "  div#wiki-action li.search a{
	border-bottom:1px solid white}
  div#wiki-foot li.search a{
	border-bottom:2px solid black}";
$ActionHi['Config'] = "  div#wiki-action li.config a{
	border-bottom:1px solid white}
  div#wiki-foot li.config a{
	border-bottom:2px solid black}";
$ActionHi['Home']   = "  div#wiki-action li.home a{
	border-bottom:1px solid white}
  div#wiki-foot li.home a{
	border-bottom:2px solid black}";

## Fontsize vars  - (Owen Briggs - concistent typography)

SDV($FontSmaller, " body{font-size: 69%}\n" );

SDV($FontBigger , " body{font-size: 93%}\n" );

## side bar functions

function NoSideBars(){
global $HTMLStylesFmt,$NoSideBars,
	$PageRightFmt,$PageLeftFmt;

	SetTmplDisplay('PageRightFmt',0);
	SetTmplDisplay('PageLeftFmt',0);
	$HTMLStylesFmt['sidebars'] = $NoSideBars;
}

function NoRightMenu(){
global $HTMLStylesFmt,$PageRightFmt,$NoRightBar;

	SetTmplDisplay('PageRightFmt',0);
	$HTMLStylesFmt['sidebars'] = $NoRightBar;
}

function NoLeftMenu(){
global $HTMLStylesFmt,$PageLeftFmt,$NoLeftBar;

	SetTmplDisplay('PageLeftFmt',0);
	$HTMLStylesFmt['sidebars'] = $NoLeftBar;
	return "";
}
function NoActionMenu(){
global $HTMLStylesFmt,$PageActionFmt;

	SetTmplDisplay('PageActionFmt',0);
	SetTmplDisplay('PageFootFmt',0);
}
function NoSearchBox(){
global $HTMLStylesFmt,$PageSearchFmt;

	SetTmplDisplay('PageSearchFmt',0);
}
function NoHeader(){
global $HTMLStylesFmt,$PageHeadFmt;

	SetTmplDisplay('PageHeadFmt',0);
	NoSearchBox();
}
function NoFooter(){
global $HTMLStylesFmt,$PageFootFmt;

	SetTmplDisplay('PageFootFmt',0);
}

## skin directives for nosections

if ($EnableSkinOptions && $EnableNoSections){

	# nosidebars
	Markup('nosidebars','directives',
		'/\\(:nosidebars:\\)/e', "NoSideBars()");

	# noright
	Markup('noright','directives',
		'/\\(:noright:\\)/e', "NoRightMenu()");

	# noleft
	Markup('noleft','directives',
		'/\\(:noleft:\\)/e', "NoLeftMenu()");

	# noaction
	Markup('noaction','directives',
		'/\\(:noaction:\\)/e', "NoActionMenu()");
	
	# no search box
	Markup('nosearchbox','directives',
		'/\\(:nosearchbox:\\)/e', "NoSearchBox()");
	
	# no header	
	Markup('nohead','directives',
		'/\\(:nohead:\\)/e', "NoHeader()");

	# no footer
	Markup('nofoot','directives',
		'/\\(:nofoot:\\)/e', "NoFooter()");
}

## Remove SideBar when...

if ($action == "edit"){
	NoSideBars();
}

## Sidebar control

if ($EnableSkinOptions && $action != "edit"){

	if( $_GET[col] != ""){
		$col = $_GET[col];
	}
	
	if($_GET[col] == "" && $_COOKIE[col] != ""){
		$col = $_COOKIE[col];
	}
	
	if( $col == "0" ){
	#NO LEFT and RIGHT column
		NoSideBars();
		setcookie("col",$col,$SkinCookieExpiration,'/');
	}
	
	if( $col == "1" ){
	#NO RIGHT column
		NoRightMenu();
		setcookie("col",$col,$SkinCookieExpiration,'/');
	}
	
	if( $col == "2" ){
	#NO LEFT column
		NoLeftMenu();
		setcookie("col",$col,$SkinCookieExpiration,'/');
	}
	
	if( $col == "3" ){
	#SHOW ALL columns - Default is in the css file
		SetTmplDisplay('PageRightFmt',1);
		SetTmplDisplay('PageLeftFmt',1);
		setcookie("col","",$SkinCookieExpiration,'/');
	}
}

## Font control

if ($EnableSkinOptions && $EnableFontOptions ){
	if( $_GET[fnt] != ""){
		$fnt = $_GET[fnt];
	}
	
	if($_GET[fnt] == "" && $_COOKIE[fnt]!= ""){
		$fnt = $_COOKIE[fnt];
	}
	
	if( $fnt == "0" ){
	#smaller font
		$HTMLStylesFmt['fontsize'] = $FontSmaller;
		setcookie("fnt",$fnt,$SkinCookieExpiration,'/');
	}
	if( $fnt == "1" ){ 
	#normal size font - default is in the css file
		setcookie("fnt","",$SkinCookieExpiration,'/');
	}
	if( $fnt == "2" ){
	#bigger size font
		$HTMLStylesFmt['fontsize'] = $FontBigger;
		setcookie("fnt",$fnt,$SkinCookieExpiration,'/');
	}
}

## Highlight for actions 
If ($EnableSkinOptions && $EnableActionHighlight){
	if( $action == "edit" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['Edit'];
	if( $action == "browse" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['Browse'];
	if( $action == "diff" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['Diff'];
	if( $action == "login" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['LogIn'];
	if( $action == "logout" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['LogOut'];
	if( $action == "config" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['Config'];
	if( $action == "search" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['Search'];
	if( $action == "home" )
		$HTMLStylesFmt['actionHigh'] = $ActionHi['Home'];
}

## search box 2 - By Hans Bracker

function SearchBox2($pagename, $opt) {
  global $SearchBoxOpt, $SearchQuery, $EnablePathInfo;
 SDVA($SearchBoxOpt, array(
    'size'   => '20', 
    'label'  => FmtPageName('$[Search]', $pagename),
    'value'  => str_replace("'", "&#039;", $SearchQuery)));
 $opt = array_merge((array)$SearchBoxOpt, (array)$opt);
 $focus = $opt['focus'];
 $opt['action'] = 'search';
 if($opt['target']) $target = MakePageName($pagename, $opt['target']); 
 else $target = $pagename;
 $out = FmtPageName(" id='wiki-search' action='\$PageUrl' method='get'>", $target);
 $opt['n'] = IsEnabled($EnablePathInfo, 0) ? '' : $target;
 $out .= "
   <input type='text' name='q' value='{$opt['value']}' id='search-textbox' size='{$opt['size']}' ";
 if ($focus) $out .= "
    onfocus=\"preval=this.value; this.value=''\" onblur=\"this.value=preval\" ";
 $out .= " />
   <input type='submit' id='search-button' value='{$opt['label']}' />";
 foreach($opt as $k => $v) {
   if ($v == '' || is_array($v)) continue;
   if ($k=='q' || $k=='label' || $k=='value' || $k=='size') continue;
   $k = str_replace("'", "&#039;", $k);
   $v = str_replace("'", "&#039;", $v);
   $out .= "
   <input type='hidden' name='$k' value='$v' />";
 }
 return "<form ".Keep($out)."
  </p></form>";
}

Markup('searchbox', '>links',
  '/\\(:searchbox(\\s.*?)?:\\)/e',
  "SearchBox2(\$pagename, ParseArgs(PSS('$1')))");

## Includes maguila.d in WikiLibDirs

$PageStorePath = dirname(__FILE__)."/maguila.d/\$FullName";
$where = count($WikiLibDirs);
if ($where>1) $where--;
array_splice($WikiLibDirs, $where, 0, array(new PageStore($PageStorePath)));

## Compatibility check for pmwiki version number

global	$VersionNum, $CompatibilityNotice;

if($VersionNum < '2001018'){
   $CompatibilityNotice = 
	"<p style='color:red'>
		$[Compatibility problem: Please upgrade to the latest pmwiki version]
	</p>";
}

## Backward compatibility for non-relative urls

if ($VersionNum < 2001900){
      Markup('{*$var}', '<{$var}', '/\\{\\*\\$/', '{$');
}