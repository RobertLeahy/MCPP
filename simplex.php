<?php


	function get_header ($file) {
	
		$num=intval(fgetc($file));
		$retr='';
		for ($i=0;$i<$num;++$i) $retr.=fgetc($file);
		return intval($retr);
	
	}


	$file=fopen('./simplex.txt','r');
	
	//	Get width
	$width=get_header($file);
	//	Get height
	$height=get_header($file);
	
	//	Create image and colours
	$image=imagecreate($width,$height);
	$black=imagecolorallocate($image,0,0,0);
	$white=imagecolorallocate($image,255,255,255);
	
	$x=0;
	$y=0;
	while (($char=fgetc($file))!==false) {
	
		imagesetpixel(
			$image,
			$x,
			$height-1-$y,
			($char==='T') ? $black : $white
		);
		
		++$y;
		
		if ($y==$height) {
		
			$y=0;
			++$x;
		
		}
	
	}
	
	imagepng($image,'./simplex.png');
	
	header('Content-Type: image/png');
	readfile('./simplex.png');


?>