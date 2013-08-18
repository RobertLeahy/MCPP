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
			
			
			function to_int ($str) {
			
				if (
					is_numeric($str) &&
					(($retr=intval($str))==floatval($str))
				) return $retr;
				
				throw new Exception('Integer format error! '.$str.' is not an integer!');
			
			}
			
			
			function get_header_int ($file) {
			
				return to_int(get_header($file));
			
			}
			
			
			function write_text ($text, $offset) {
				
				global $image;
				global $colours;
				
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
					$colours[0],
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
				$start_z=get_header($file);
				if ($start_z!=='') $start_z=to_int($start_z);
				$width=get_header_int($file);
				$height=get_header_int($file);
				
				//	Create image and colours
				$image=imagecreate($width,$height);
				$colours=array();
				for ($i=0;$i<256;++$i) {
				
					$colours[]=imagecolorallocate(
						$image,
						$i,
						$i,
						$i
					);
				
					/*if ($i===0) {
					
						$colours[]=imagecolorallocate(
							$image,
							255,
							255,
							255
						);
					
					} else if ($i===1) {
					
						$colours[]=imagecolorallocate(
							$image,
							127,
							127,
							127
						);
					
					} else if ($i===2) {
					
						$colours[]=imagecolorallocate(
							$image,
							0,
							0,
							0
						);
				
					} else if ($i<52) {
					
						$colours[]=imagecolorallocate(
							$image,
							0,
							intval((1-(floatval($i-3)/floatval(51-3)))*128),
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
					
					}*/
				
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
				
				
				/*$offset=5;
				$offset+=write_text(
					'Seed: '.$seed,
					$offset
				)+5;
				$offset+=write_text(
					'1px = '.$scale.' block'.(($scale!==1) ? 's' : ''),
					$offset
				)+5;*/
				$real_width=$width*$scale;
				$end_x=$start_x+$real_width;
				/*$offset+=write_text(
					'X: '.$start_x.' to '.$end_x.' ('.round($real_width/1000,1).'km)',
					$offset
				)+5;*/
				$real_height=$height*$scale;
				$end_z=$start_z+$real_height;
				/*if ($start_z!=='') write_text(
					'Z: '.$start_z.' to '.$end_z.' ('.round($real_height/1000,1).'km)',
					$offset
				);*/
				
				$output='simplex_'.$seed.'_'.$scale.'_'.$start_x.(
					($start_z!=='')
						?	'_'.$start_z
						:	''
				).'.png';
				
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
				<?php	if ($start_z!==''):	?>
				-
				Z: <?php	echo(htmlspecialchars($start_z));	?>
				-
				<?php	echo(htmlspecialchars($end_z));	?>
				<?php	endif;	?>
				
			</a>
			
		</div><?php
				
				foreach ($colours as $x) imagecolordeallocate($image,$x);
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