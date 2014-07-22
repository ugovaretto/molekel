<?php if (!defined('PmWiki')) exit();

/*  Copyright 2005 Christophe David (www.christophedavid.org)
    This file is VisitorsLogging.php; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This script creates a log file for the wiki usage.

    To use this script, simply copy it into the cookbook/ directory
    and add the following line to config.php or a per-page/per-group
    customization file.

        include_once('cookbook/VisitorsLogging.php');
*/

################################################################################

SDV($VisitorsLoggingDeleteLogsOlderThanDays, 60);
SDV($VisitorsLoggingDirectory,              "$WorkDir/VisitorsLogging");

# this should be $WikiDir instead of $WorkDir, but this "PageStore object"
# certainly desserves some more documentation.
# Anyway, by default they point to the same directory.

################################################################################
################################################################################
################################################################################

define(VISITORS_LOGGING, '1.0');

if (   (is_dir($VisitorsLoggingDirectory) == true)
    || (mkdirp($VisitorsLoggingDirectory) == true)
   )
   {
   $TimeNow = time();
   $VisitorsLoggingFileName =   $VisitorsLoggingDirectory
                              . '/'
                              . strftime("%Y%m%d.txt", $TimeNow);

   # cleanup old logs once a day
   if (file_exists($VisitorsLoggingFileName) == false)
      {
      if (($DirectoryHandle = opendir($VisitorsLoggingDirectory)) == true)
         {
         while (($file = readdir($DirectoryHandle)) == true)
            {
            if (is_dir($file) == false)
               {
               $FileFullPath = $VisitorsLoggingDirectory . '/' . $file;
               $FileStat     = stat($FileFullPath);
               $FileAge      = $TimeNow - $FileStat[9];
               if ($FileAge > ($VisitorsLoggingDeleteLogsOlderThanDays * 86400))
                  {
                  unlink($FileFullPath);
                  }
               }
            }
         closedir($DirectoryHandle);
         }
      }

   if (($FileHandle = @fopen($VisitorsLoggingFileName, 'a')) != FALSE)
      {
      fwrite($FileHandle,
             sprintf("%s %-15s %-8s %s.%s\n",
                     strftime("%Y/%m/%d %H:%M:%S", $TimeNow),
                     $_SERVER['REMOTE_ADDR'],
                     $action,
                     FmtPageName('$Group', $pagename),
                     FmtPageName('$Name',  $pagename)
                    )
            );
      fclose($FileHandle);
      }
   }
################################################################################
?>
