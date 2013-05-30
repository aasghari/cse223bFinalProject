<!DOCTYPE html>

<html>
<head>
    <meta charset="utf-8" />
    <title></title>
    <style type="text/css">
        #Text1 {
            width: 323px;
        }
        .auto-style1 {
            height: 23px;
        }
        #listTextArea {
            height: 116px;
            width: 269px;
        }
    </style>
</head>
<body>
    
    <p> <center><img src="img.png"  height="200" width="200"></td></center></p>
    <p> <center><font size="20">Gas Friends</font></td></center></p>
    <center><form name="myForm" form  method="POST">
    <p>Station Name: <input id="stationNameTXT" type="text" /></p>
    <p> Gas Price: <input id="priceTXT" type="text" /></p>
    <p> <input id="Submit1" type="submit"  value="submit" /></p>
    <?php if($_POST["submit"]= Submit) exec("../serverClient/ThreadsClient"); ?>
    <p>Current Prices</p>
    <p> <textarea id="listTextArea" name="S1"></textarea></p>
    </form></center>
             
 
</body>
</html>



<!--<?php exec("../serverClient/ThreadsClient"); ?>-->
