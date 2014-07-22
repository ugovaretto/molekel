<?php if (!defined('PmWiki')) exit ();

# Version date
$RecipeInfo['YouTube']['Version'] = '2009-08-26';

Markup('bliptv', '<img', "/\\(:bliptv (.*?)\\s*:\\)/e", "ShowBlipTv('$1')");

function ShowBlipTv($url) 
{
  ## object tag
  #<embed src="http://blip.tv/play/AYGa3HgA" 
  #type="application/x-shockwave-flash" width="864" height="510"
  #allowscriptaccess="always" allowfullscreen="true"></embed>

  #
  # by molekel, molekeladmin@gmail.com
  #

  
  $out = "\n<embed src='http://blip.tv/play/$url' type='application/x-shockwave-flash' width='555' height='340' allowscriptaccess='always' allowfullscreen='true'></embed>\n";
  return Keep( $out );

}