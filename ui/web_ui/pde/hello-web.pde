float radius = 50.0;
int X, Y;
int nX, nY;
int delay = 16;
int cdelay = 20;

void setup(){
  size( 200, 200 );
  strokeWeight( 1 );
  frameRate( 15 );

  X = width / 2;
  Y = height / 2;
  nX = X;
  nY = Y;
}

void draw() {
  radius = radius + sin(frameCount / 4);

  X += (nX - X)/delay;
  Y += (nY - Y)/delay;

  background(100);

  fill(0, 121, 80 + 100*sin(frameCount) );

  stroke(0, 121, 80 + 100*sin(frameCount) );

  ellipse(X, Y, radius, radius);
}

void mouseMoved() {
  //redraw();
  nX = mouseX;
  nY = mouseY;
}