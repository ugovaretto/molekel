<?php if (!defined('PmWiki')) exit ();

/*  copyright 2007 Jon Haupt. 
    Build on code from swf.php copyright 2004 Patrick R. Michaud
    and from quicktime.php copyright 2006 Sebastian Siedentopf.
    This file is distributed under the terms of the GNU General Public 
    License as published by the Free Software Foundation; either 
    version 2 of the License, or (at your option) any later version.  

    This module enables embedding of YouTube movies into
    wiki pages. simply use (:youtube whatever:) where "whatever" is 
    replaced with the little funny encrypted-looking code of numbers,
    letters, and symbols that is attached to the end of YouTube URLs.
    So for example if the URL to share a video is:
    
    http://youtube.com/watch?v=tiljk_MemVM
    
    then you'd write (:youtube tiljk_MemVM:).

*/
# Version date
$RecipeInfo['YouTube']['Version'] = '2007-02-06';

Markup('youtube', '<img', "/\\(:youtube (.*?)\\s*:\\)/e", "ShowYouTube('$1')");

function ShowYouTube($url) 
{
  ## object tag
  #    $out = "\n<object type='application/x-shockwave-flash' style='width:425px; height:350px;' ";
  #    $out .= "data='http://www.youtube.com/v/$url'>";
  #    ## transparent so you can see through it if necessary
  #    $out .= "\n  <param name='wmode' value='transparent' />";
  #    ## move param
  #    $out .= "\n  <param name='movie' value='http://www.youtube.com/v/$url' />";
  #    $out .= "\n</object>";
  #    return Keep($out);

  #
  # by Jabba Laci, jabba.laci@gmail.com
  #
  $out = "\n<object width='560' height='340'>\n".
         "<param name='movie' value='http://www.youtube.com/v/$url'></param>\n".
         "<param name='allowFullScreen' value='true'></param>\n".
         "<param name='allowscriptaccess' value='true'></param>\n".
         "<param name='wmode' value='transparent'></param>\n".
         "<embed src='http://www.youtube.com/v/$url' type='application/x-shockwave-flash' allowscriptaccess='always' allowfullscreen='true' width='560' height='340'></embed>\n".
         "</object>";
  return Keep($out);
}
