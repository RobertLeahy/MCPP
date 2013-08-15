<!doctype html>

<html xmlns="http://www.w3.org/1999/xhtml">

	<head>
	
		<meta http-equiv="content-type" content="application/xhtml+xml; charset=UTF-8" />
	
	</head>
	
	<body>

		<?php


			define('TEXT_SIZE',18);
			define('URL','http://rleahy.ca/MCPP/');


			function get_header ($file) {
			
				$str='';
				
				for (;;) {
				
					$char=fgetc($file);
					
					if ($char===false) throw new Exception('File format error!');
					
					if ($char==="\0") break;
					
					$str.=$char;
				
				}
				
				return $str;
			
			}
			
			
			function get_header_int ($file) {
			
				$str=get_header($file);
				
				if (
					is_numeric($str) &&
					(($retr=intval($str))==floatval($str))
				) return $retr;
				
				throw new Exception('File format error! '.$str.' is not an integer!');
			
			}
			
			
			function write_text ($text, $offset) {
				
				global $image;
				global $text_colour;
				
				$bbox=imagettfbbox(
					TEXT_SIZE,
					0,
					'Arial',
					$text
				);
				
				$height=abs($bbox[5]-$bbox[1]);
				
				imagettftext(
					$image,
					TEXT_SIZE,
					0,
					5,
					$offset+$height,
					$text_colour,
					'Arial',
					$text
				);
				
				return $height;
			
			}
			
			
			$dir=opendir('./');
			while (is_string($filename=readdir($dir))) if (preg_match(
				'/\\.txt$/iu',
				$filename
			)) {

				$file=fopen('./'.$filename,'r');
				
				$seed=get_header($file);
				$scale=get_header_int($file);
				$start_x=get_header_int($file);
				$start_z=get_header_int($file);
				$width=get_header_int($file);
				$height=get_header_int($file);
				
				//	Create image and colours
				$image=imagecreate($width,$height);
				$text_colour=imagecolorallocate(
					$image,
					0,
					0,
					0
				);
				$colours=array();
				for ($i=0;$i<256;++$i) {
				
					if ($i<52) {
					
						$colours[]=imagecolorallocate(
							$image,
							0,
							intval((1-(floatval($i)/floatval(51)))*128),
							255
						);
					
					} else if ($i<154) {
					
						//	Start: Green (102,204,0)
						//	End: Yellow (255,255,0)
						
						$dist=floatval($i-52)/floatval(153-52);
						
						$colours[]=imagecolorallocate(
							$image,
							intval(((255-102)*$dist)+102),
							intval(((255-204)*$dist)+204),
							0
						);
					
					} else {
					
						//	Start: Yellow (255,255,0)
						//	End: Red (255,0,0)
						
						$colours[]=imagecolorallocate(
							$image,
							255,
							intval((1-(floatval($i-154)/floatval(255-154)))*255),
							0
						);
					
					}
				
				}
				
				$x=0;
				$y=0;
				while (($char=fgetc($file))!==false) {
				
					imagesetpixel(
						$image,
						$x,
						$height-1-$y,
						$colours[ord($char)]
					);
					
					++$y;
					
					if ($y===$height) {
					
						$y=0;
						++$x;
					
					}
					
				}
				
				
				putenv('GDFONTPATH='.realpath('.'));
				
				
				$offset=5;
				$offset+=write_text(
					'Seed: '.$seed,
					$offset
				)+5;
				$offset+=write_text(
					'1px = '.$scale.' blocks',
					$offset
				)+5;
				$real_width=$width*$scale;
				$end_x=$start_x+$real_width;
				$offset+=write_text(
					'X: '.$start_x.' to '.$end_x.' ('.round($real_width/1000,1).'km)',
					$offset
				)+5;
				$real_height=$height*$scale;
				$end_z=$start_z+$real_height;
				write_text(
					'Z: '.$start_z.' to '.$end_z.' ('.round($real_height/1000,1).'km)',
					$offset
				);
				
				$output='simplex_'.$seed.'_'.$scale.'_'.$start_x.'_'.$start_z.'.png';
				
				imagepng($image,'./'.$output);
				
		?><div>
		
			<a target="_new" href="<?php	echo(htmlspecialchars(URL.rawurlencode($output)));	?>">
			
				Seed: <?php	echo(htmlspecialchars($seed));	?>
				-
				Scale: <?php	echo(htmlspecialchars($scale));	?>
				-
				X: <?php	echo(htmlspecialchars($start_x));	?>
				-
				<?php	echo(htmlspecialchars($end_x));	?>
				-
				Z: <?php	echo(htmlspecialchars($start_z));	?>
				-
				<?php	echo(htmlspecialchars($end_z));	?>
				
			</a>
			
		</div><?php
				
				foreach ($colours as $x) imagecolordeallocate($image,$x);
				imagecolordeallocate($image,$text_colour);
				imagedestroy($image);
				
				fclose($file);
				rename(
					'./'.$filename,
					'./processed/'.$filename
				);
				
			}


		?>
		
	</body>
	
</html>