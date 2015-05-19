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
 const MAX_RES            = 512; //1000;
 const MIN_RES            = 64;
 const DEFAULT_RES        = 256; //300;
 const DEFAULT_INTENSITY  = 30;
 const DEFAULT_BACKGROUND = 0;

 // VisColorModes
 public $colour = new VisColorModes();

 // the message digest based on which the fractal is going to be created
 private $checksum;
 public function getChecksum(){
  return $this->checksum;
 }
 public function setChecksum(string $c){
  // The checksum needs to be a string of at least 32 and at most 128 hex digits.
  // The value longer then 128 would result in a foggy image.
  // The length of 32 match the size of MD5 and 128 - the size of SHA512.
  if(preg_match('/^[0-9a-fA-F]{32,128}$/', $c)){
   $this->checksum = strtolower($c);
   return $this->checksum;
  }else{
   // In case the provided string is not a valid message digest - the SHA256 is aplied.
   return $this->setChecksum(hash('sha256', "$c");
  }
 }

 public $isTransparent = true;

 // the size of the image to be created
 private $res = self::DEFAULT_RES;
 public function getRes(){
  return $this->res;
 }
 public function setRes(int $r){
  if($r > self::MAX_RES){
   $r = self::MAX_RES;
  }
  if($r < self::MIN_RES){
   $r = self::MIN_RES;
  }
  $this->res = $r;
  return $r;
 }

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
