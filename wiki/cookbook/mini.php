<?php if(!defined('PmWiki'))exit;
/**
	A mini square thumbnail generator for PmWiki
	Written by (c) Petko Yotov 2006-2008

	This script is POSTCARDWARE, if you like it or use it,
	please send me a postcard. Details at
	http://galleries.accent.bg/Cookbook/Postcard

	This text is written for PmWiki; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published
	by the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version. See pmwiki.php for full details
	and lack of warranty.

	This text is partly based on the ThumbList2 picture gallery
	and on the PmWiki upload.php script.

	Copyright 2006-2008 Petko Yotov http://5ko.fr
	Copyright 2004-2007 Patrick R. Michaud http://www.pmichaud.com
*/
$RecipeInfo['Mini']['Version'] = '20090221';

SDVA($Mini, array(
	'EnableLightbox' => 0,
	'Px' => 100,'Py' => 0,'Quality' => 90,
	'CropXPos' => 0.5, 'CropYPos' => 0.5,
	'LbMaxH'=> 0,'LbMaxW'=> 0,
	'BgColor' => array(0xff, 0xff, 0xff),
	'ImgFmt' => '<img class="mini" src="%1$s" title="%2$s" alt="%2$s" border="0" />',
	'LinkFmt' => '<a href="%2$s" class="minilink" %3$s>%1$s</a>',

	'ImTypes' => array(1=>"gif",2=>"jpeg",3=>"png",15=>"wbmp",16=>"xbm"),
	'ImRx' => array("/\\.(gif|png|jpe|jpe?g|wbmp|xbm)$/", "!^th\\d+---!"),

	'LbJS' => '<script type="text/javascript" src="%1$s/prototype.js"></script>
<script type="text/javascript" src="%1$s/scriptaculous.js?load=effects"></script>
<script type="text/javascript"><!--
LightboxDirUrl = "%1$s";//--></script>
<script type="text/javascript" src="%1$s/lightbox.js"></script>
<link rel="stylesheet" href="%1$s/lightbox.css" type="text/css" media="screen"/>',
	'LbUrl'=>"\$FarmPubDirUrl/lb",
	'LbRels'=>array('','%s[mini]','%s[mini%d]','%s'),
	'EnableCache' => 0,
	'CacheFilename' => '.%s.mini-cache.txt',
));

SDVA($HandleActions, array('mini'=>'HandleMini','purgethumbs'=>'HandlePurgeMini'));

Markup('Mini:','<links',
  "/\\b([Mm]ini:)([^\\s\"\\|\\[\\]]+)(\"([^\"]*)\")?/e",
  "Keep(LinkMini(\$pagename,'$1','$2','$4','$1$2'),'L')");

Markup('(:mini:)', 'directives',
	'/\\(:mini (.+):\\)/e',	"MiniConf(PSS('$1'))");

function LinkMini($PN, $imap, $path, $alt, $txt, $fmt=NULL)
{
	global $FmtV, $UploadFileFmt, $LinkUploadCreateFmt, $UploadUrlFmt, $PCache,
		$UploadPrefixFmt, $EnableDirectDownload, $Mini, $HTMLHeaderFmt, $Charset;
	if(! function_exists('imagecreate'))
		return "Mini: PHP-GD image library not found. Exiting.";
	static $cnt = 0; $cnt++;

	$lb = sprintf(@$Mini['LbRels'][ $Mini['EnableLightbox'] ], 'lightbox', $cnt);
	if($lb>'')$HTMLHeaderFmt['lightbox'] = sprintf($Mini['LbJS'], $Mini['LbUrl']);
	$ptime = $PCache[$PN]['time'];

	$test_cache = ($Mini['EnableCache'] && !@$_POST['preview']);
	if($test_cache)
	{
		$cachedir = FmtPageName($UploadFileFmt, $PN);
		$cachefile =  sprintf("$cachedir/{$Mini['CacheFilename']}", $PN);
		if(!@$_GET['recache'] && !(isset($Mini['Cache'][0]) && isset($Mini['Cache'][$cnt])))
		{
			$Mini['Cache'][0] = 1;
			if(file_exists($cachefile) && filemtime($cachefile) >= $ptime)
			{
				$cache = implode('', file($cachefile));
				preg_match_all("/<(Mini(\\d+))>(.*?)<\\/\\1>/", $cache, $m);
				foreach($m[2] as $i=>$x)$Mini['Cache'][$x] = $m[3][$i];
			}
		}
		if(isset($Mini['Cache'][$cnt])) return $Mini['Cache'][$cnt];
	}
	$cache_ok = 1;

	$mH=@$Mini['LbMaxH'];$mW=@$Mini['LbMaxW'];
	if (preg_match('!^(.*)/([^/]+)$!', $path, $m)) {
		$PN = MakePageName($PN, $m[1]);
		$path = $m[2];
	}
	$uploadurl = FmtPageName(IsEnabled($EnableDirectDownload,1)
			? "$UploadUrlFmt$UploadPrefixFmt/"
			: "\$PageUrl?action=download&amp;upname=",
		$PN);
	$flist = array();
	if(preg_match("/(^|,)[!-]|[\\*\\?]/", $path))
	{
		$uploaddir = FmtPageName($UploadFileFmt, $PN);
		if($dirp=@opendir($uploaddir))
		{
			while (($f=readdir($dirp))!==false)if($f{0}!='.')$flist[$f]=$f;
			closedir($dirp);
		}
		$flist = MatchNames($flist, array_merge($Mini['ImRx'], array($path)));
		natcasesort($flist);
	}

	foreach(explode(',', $path) as $v)$mylist[$v] = $v;
	$mylist = array_merge(preg_grep("/^[!-]|[\\*\\?]/", $mylist, PREG_GREP_INVERT), $flist);

	$html = array();
	$htmlH = $htmlF = '';
	foreach($mylist as $file=>$v)
	{
		$upname = MakeUploadName($PN, $v);
		$fpath = FmtPageName("$UploadFileFmt/$upname", $PN);
		$picurl = PUE("$uploadurl$file");

		if(!file_exists($fpath))
		{
			$FmtV['$LinkText'] = $file;
			$FmtV['$LinkUpload'] =
				FmtPageName("\$PageUrl?action=upload&amp;upname=$upname", $PN);
			$html[] = FmtPageName($LinkUploadCreateFmt, $PN);
			continue;
		}
		list($w, $h, $t) = @getimagesize($fpath, $info);
		if(!isset($Mini['ImTypes'][$t]))
		{
			$html[] =  LinkIMap($PN, "Attach:", $file, $alt, "Attach:$file", $fmt);
			continue;
		}
		$ic = @$Mini['GetIPTC'];
		if($ic>'' && isset($info['APP13']) && trim($alt)=='')
		{
			$iptc = @iptcparse($info['APP13']);
			$xiptc = @$iptc["2#120"][0];
			if(strlen($ic)>3 && $ic!=$Charset && function_exists('iconv'))
				$xiptc = iconv($ic, "$Charset//IGNORE", $xiptc);
		}
		unset($iptc); unset($info);
		$stat = stat($fpath);
		$kb = round($stat['size']/1024);

		$mupname = "th00---$upname.jpg";
		$mpath = FmtPageName("$UploadFileFmt/$mupname", $PN);

		if(file_exists($mpath) && filemtime($mpath)>=$stat['mtime'])
			$miniurl = PUE("$uploadurl$mupname");
		else
		{
			$miniurl = PUE(FmtPageName("{\$PageUrl}?action=mini&amp;upname=$upname", $PN));
			NoCache();
			$cache_ok = 0;
		}
		if(trim($alt) == '-') $xalt='';
		elseif($alt>'') $xalt=str_replace('"', "&quot;", $alt);
		elseif(trim($xiptc)>'')$xalt=$xiptc;
		else $xalt = "$v: {$w}x$h, {$kb}k";
		if(IsEnabled($Mini['EnableHeaderFooter'], 0) && strpos($xalt, '|')!==false)
		{
			list($htmlH, $htmlF) = explode('|', $xalt, 2);
			$xalt = trim("$htmlH $htmlF");
		}
		$out = sprintf($Mini['ImgFmt'], $miniurl, $xalt);
		if($imap == 'Mini:') # links enabled
		{
			$rel='';
			if($lb>'' && ($mH==0 || $h<=$mH) && ($mW==0 || $w<=$mW))
			{
				$HTMLHeaderFmt['lightbox'] = sprintf($Mini['LbJS'], $Mini['LbUrl']);
				$rel = "rel='$lb' title=\"$xalt\"";
			}
			$out = sprintf($Mini['LinkFmt'], $out, $picurl, $rel);
		}
		$html[] = $out;
	}
	$html = implode(' ', $html);
	if($htmlH) $html = "<span class='miniH'>$htmlH</span> $html";
	if($htmlF) $html .= " <span class='miniF'>$htmlF</span>";
	if($test_cache)
	{
		if($cache_ok)
		{
			$mode = ($cnt==1)? 'w+' : 'a+';
			mkdirp($cachedir);
			if ($handle = @fopen($cachefile, $mode))
			{
				@fwrite($handle, "<Mini$cnt>$html</Mini$cnt>\n");
				fclose($handle);
			}
		}
		elseif(file_exists($cachefile)) unlink($cachefile);
	}
	return $html;
}

function MiniConf($args)
{
	global $Mini; $opt = ParseArgs($args);
	foreach($Mini as $k=>$v)
	{
		if(!preg_match("/^(P([xy])|Crop([XY]Pos))$/", $k, $m) ) continue;
		if(isset($opt[$m[2]])) $Mini[$k] = intval($opt[$m[2]]);
		elseif(isset($opt[$m[3]])) $Mini[$k] = min(max(floatval($opt[$m[3]]), 0), 1);
	}
}

function HandleMini($PN, $auth="read")
{
	global $Mini, $WorkDir, $UploadFileFmt, $UploadDir, $UploadPrefixFmt;
	$page = RetrieveAuthPage($PN,$auth,1, READPAGE_CURRENT);# ask for pw if needed
	$q = preg_replace('/\\(:mini (.+):\\)/e',	"MiniConf(PSS('$1'))", $page['text']);

	$upname = MakeUploadName($PN, $_REQUEST['upname']);
	$mupname = "th00---$upname.jpg";
	$fpath = FmtPageName("$UploadFileFmt/$upname", $PN);
	$mpath = FmtPageName("$UploadFileFmt/$mupname", $PN);

	if(!file_exists($fpath)){Abort("? file '$fpath' not found."); exit;}
	if(!file_exists($mpath) || filemtime($mpath)<filemtime($fpath))
	{
		list($w, $h, $t) = @getimagesize($fpath);
		if(!isset($Mini['ImTypes'][$t])){Abort("? format $t not supported."); exit;}
		MiniCreate($fpath,$mpath,$Mini['Px'],$Mini['Py'],$Mini['Quality'],$w,$h,$t);
	}
	$_REQUEST['upname'] = $mupname;
	HandleDownload($PN);
}
function MiniCreate($fpath,$mpath,$w,$h,$quality,$W,$H,$T)
{
	global $Mini;
	list($rr, $gg, $bb) =  $Mini['BgColor'];
	$gd2 = function_exists('imagecreatetruecolor');
	$imcopy = ($gd2)?'imagecopyresampled':'imagecopyresized';
	$imcreate=($gd2)?'imagecreatetruecolor':'imagecreate';
	$fcreate = "imagecreatefrom".$Mini['ImTypes'][$T];
	$img = $fcreate($fpath);
	if (!@$img){return;}

	if($h==0)$h=$w;

	$nimg = $imcreate($w, $h);
	imagefill($nimg, 0, 0, imagecolorallocate($nimg, $rr, $gg, $bb));

	$percent = max(1, min($H/$h, $W/$w));
	$_h = round($percent*$h);
	$_w = round($percent*$w);

	
	$sW = min($W,$_w); #source width
	$sH = min($H,$_h);
	$sY = max(0, round(($H-$_h)*$Mini['CropYPos']));
	$sX = max(0, round(($W-$_w)*$Mini['CropXPos']));

	$tW = min($w, $W); #target width
	$tH = min($h, $H);
	$tY = max(0, round(($h-$H)/2));
	$tX = max(0, round(($w-$W)/2));

	$imcopy($nimg,$img,$tX,$tY,$sX,$sY,$tW,$tH,$sW,$sH);

	imagedestroy($img);
	if(function_exists('imageconvolution'))
		imageconvolution($nimg, array(array(-1,-1,-1),array(-1,16,-1),array(-1,-1,-1)),8,0);
	imagejpeg($nimg,$mpath,$quality);
	imagedestroy($nimg);
}
function HandlePurgeMini($PN, $auth='edit')
{
	RetrieveAuthPage($PN, $auth, 1, READPAGE_CURRENT);
	global $UploadDir, $UploadPrefixFmt, $Mini;
	$udir = FmtPageName("$UploadDir$UploadPrefixFmt", $PN);
	$cachefile =  preg_quote(sprintf($Mini['CacheFilename'], $PN));
	if ($dirp = @opendir($udir))
	{
		while (($file=readdir($dirp)) !== false)
			if (preg_match("/^(th\\d+---|$cachefile$)/", $file))
				unlink("$udir/$file");
		closedir($dirp);
	}
	if(@$_REQUEST['redirect']>'')Redirect(MakePageName($PN, $_REQUEST['redirect']));
	else Redirect($PN, '{$PageUrl}?action=upload');
}
if(!function_exists('MatchNames')){function MatchNames($l,$p){return MatchPageNames($l,$p);}}


