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

class VisPoint{
 // 0..255 ~ 8b
 public $x;

 public function getX(){
  return $this->x&0xff;
 }

 // 0..255 - 8b
 public $y;
  
 public function getY(){
  return $this->y&0xff;
 }

 // This is intended to return both coordinates at one int value
 // where the higher bits corresponds to the $x coordinate while
 // the lower bits are storing the value of $y.
 public function getCoordinates(){
  return ($this->x<<8)|$this->y;
 }

 public function setCoordinates(int $c){
  $c = ((int)$c)&0xffff;
  $this->x = $c>>8;
  $this->y = $c&0xff;
  return $this;
 }

 // This is intended to set the color of the point based on the
 // given color mode and its location.
 public function setColor(VisColorModes $m){
  $this->a = 128;
  if($m == VisColorModes::GREYSCALE){
   // Greyscale color mode: from black to white horizontally
   $this->r = $this->g = $this->b = $this->x;
  }else if($m == VisColorModes::FOURCOLOR){
   // Four color mode: red, green, blue and white
   if($this->x < 128){
    $this->r = 0;
    if($this->y < 128){
     $this->g = 0;
     $this->b = 255;
    }else{
     $this->g = 255;
     $this->b = 0;
    }
   }else{
    $this->r = 255;
    if($this->y < 128){
     $this->g = $this->b = 0;
    }else{
     $this->g = $this->b = 255;
    }
   }
  }else{
   // default: smooth transition with red, green, blue and white on the corners
   $this->r = $this->x;
   $this->g = $this->y;
   if($this->x > $this->y){
    $this->b = 255-$this->x+$this->y;
   }else{
    $this->b = 255+$this->x-$this->y;
   }
  }
 }

 // 0..255 - 8b
 public $r;
 // 0..255 - 8b
 public $g;
 // 0..255 - 8b
 public $b;

 // 0..255 - 8b
 public $a;
}

class VisMatrix{
 private $m;

 // $c ~ /^[0-9a-f]{0,128}$/
 public function __construct(string $c){
  // the arry would be half-filled but
  // the calculation at such is expected to be trivial
  $this->m = array_fill(0, 16, 0);
  if((strlen($c)<64)&&((strlen($c)&3)!=0)){
   $c .= str_repeat('0', 4-(strlen($c)&3));
  }
  for($y = 0; ($y<61)&&($y<strlen($c)); $y+=4){
   $this->setRow($y>>2, (int) hexdec(substr($c, $y, 4)));
  }
 }

 // $x in 0..15
 // $y in 0..15
 public function getByte(int $x, int $y){
  return ((1<<(((int)$x)&0xf))&$this->getRow($y)) != 0;
 }

 // $y in 0..15
 public function getRow(int $y){
  return $this->m[((int)$y)&0xf]&0xffff;
 }

 private function setRow(int $y, int $r){
  //$y = ((int)$y)&0xf;
  $r = ((int)$r)&0xffff;
  $this->m[$y] = $r;
  return $r;
 }

 // $r in 0..255
 public function move(int $r){
  $r = ((int)$r)&0xffff;
  $res = 0;
  foreach($this->m as $row){
   $res += ($row*$r)&0xffff;
  }
  return $res&0xffff;
 }
  
 public function movePoint(VisPoint $p){
  return $p->setCoordinates($this->move($p->getCoordinates()));
 }
}

class VisPrint{
 const MAX_RES            = 256;
 const MIN_RES            = 256;
 const DEFAULT_RES        = 256;
 const DEFAULT_INTENSITY  = 30;

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

 //Background can be black or white. 
 public $isBackgroundBlack = true;

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
  $im = imagecreatetruecolor($this->getRes(), $this->getRes());
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
