<?php if (!defined('PmWiki')) exit ();

# Version date
$RecipeInfo['SlideShare']['Version'] = '2009-08-31';

Markup('slideshare', '<img', "/\\(:slideshare (.*?)\\s+(.*?)\\s*:\\)/e", "ShowSlideShare('$1', '$2')");

function ShowSlideShare($url,$title) 
{
  $out = "\n<embed src='http://static.slidesharecdn.com/swf/ssplayer2.swf?doc=$url&stripped_title='$title'" . 
  	  " type='application/x-shockwave-flash' allowscriptaccess='always' allowfullscreen='true' width='555' height='463'></embed>\n";
  return Keep( $out );
}
