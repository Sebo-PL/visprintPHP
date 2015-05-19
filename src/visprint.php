<?php
/*
*/
class VisColorModes extends SplEnum{
 const __default = self::COLORFULL;

 // semi greyscale:
 const GREYSCALE = 0;
 // default colorfull image:
 const COLORFULL = 1;
 // 4 different color areas:
 const FOURCOLOR = 2;
}

class VisPrint{
 const MAX_RES            = 1000;
 const DEFAULT_RES        = 256; //300;
 const MAX_CHECKSUM       = 128;
 const DEFAULT_INTENSITY  = 30;
 const DEFAULT_BACKGROUND = 0;

 // VisColorModes
 public $colour = new VisColorModes();

 public $isTransparent = true;

 private $pic = array_fill(0, self::DEFAULT_RES, array_fill(0, self::DEFAULT_RES, array(0, 0, 0)));

 private $eqns = array_fill(0, self::MAX_CHECKSUM/8, array_fill(0, 6, 0.0));

 private $hashd = array_fill(0, self::MAX_CHECKSUM/8, array_fill(0, 6, 0));

 public function __construct($h){
   if(is_string($h)){
    if(preg_match('/^[0-9a-fA-F]*$/', $h)){
     $this->init($h);
    }else{
     $this->init(hash('sha256', $h));
    }
   }else{
    $this->init(json_encode($h));
   }
  }

 private function init($s){
  }

 public function getPNG(){
   $im = imagecreatetruecolor(256, 256);
   imagealphablending($im, !$this->isTransparent);
   imagesavealpha($im, $this->isTransparent);
   ob_start();
   imagepng($im);
   $im_data = ob_get_contents();
   ob_end_clean();
   imagedestroy($im);
   return $im_data;
  }

 public function draw(){
   $im_data = $this->getPNG();
   if(headers_sent()){
    //TODO: raise exception
   }else{
    header('Content-type: image/png');
    header('Content-Length: '.strlen($im_data));
    die($im_data);
    //ob_end_flush();
   }
  }
}
?>
