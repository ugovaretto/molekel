<?php
/**
  PmWiki module to generate a that allows you to send email to
  a person specified in the Wiki.
  Copyright (C) 2004  Nils Knappmeier <nk@knappi.org>
  This is GPL, only it seems odd to include a license in a programm
  that has twice the size of the license.

  Modifications to work with PmWiki 2.0, 
    Copyright (c) 2005 by Patrick R. Michaud <pmichaud@pobox.com>

  Revision 1.1	20060521	Nils Knappmeier	
	  Changed form-method to "post" in order to work correctly with Safari
*/


XLSDV('en', array(
  'MF' => '',
  'MFsuccess' => 'Message has been sent successfully.',
  'MFfailure' => 'Message could not be sent.',
  'MFerror' => 'An error has occurred.'));

$r = @$_REQUEST['mailform'];
$MailFormResult = FmtPageName("$[MF$r]", $pagename);


# This is the default form, as a table.  The '$1' is replaced with
# the word that comes after the mailform: markup -- the rest are
# straightforward $...Fmt substitutions.
SDV($MailFormFmt,"<form action='\$PageUrl' method='post'>
  <input type='hidden' name='pagename' value='\$FullName'>
  <input type='hidden' name='address' value='\$1'/>
  <input type='hidden' name='action' value='mailform'/>
  <table>
    <tr><td colspan='3'>\$MailFormResult</td></tr>
    <tr><td>$[Your Address:]</td>
      <td><input type='text' size='20' name='sender' value=''/></td>
      <td><input type='submit' name='send' value='$[Send]'></td></tr>
    <tr><td>$[Subject:]</td>
      <td colspan=2>
        <input type='text' size='20' value='' name='subject'/></td></tr>
    <tr><td>$[Message:]</td>
      <td colspan='2'><textarea name='text' cols='41' rows='10'></textarea>
      </td></tr></table></form>");

# This defines the mailform: markup -- it's just a straight text
# substitution.  
Markup('mailform', '>links', 
  '/\\bmailform:(\\w+)/',
  FmtPageName($MailFormFmt, $pagename));

# These define what happens after someone has submitted a message.
# The variables are the header and footer for the email message,
# while HandleMailForm sends the message according to the
# value of 'address' in the request.
SDV($MailFormHeader,"");
SDV($MailFormFooter,
   "\n-------------------------------------------\n"
  ."This message was sent by the PmWiki MailForm at $ScriptUrl\n");
SDV($MailFormDefaultSender,"");

$HandleActions['mailform'] = 'HandleMailForm';

function HandleMailForm($pagename) {
  global $MailFormAddresses, $MailFormHeader, $MailFormFooter,
    $MailFormDefaultSender;

  $to = $MailFormAddresses[$_REQUEST['address']];
  $from = $_REQUEST['sender'];
  $subject = $_REQUEST['subject'];
  $text = $MailFormHeader.stripmagic($_REQUEST['text']).$MailFormFooter;

  if (!$from) $from=$MailFormDefaultSender;
  if (!$to || !$_REQUEST['text']) $msg = 'error';
  else if (mail($to, $subject, $text, "From: $from")) $msg = 'success';
  else $msg = 'failure';
  header("Location: $ScriptUrl?pagename=$pagename&mailform=$msg");
}

?>
