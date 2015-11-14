/*

Creates and saves a simple obj file of format:

n
v 1.0 2.0 0.0
v 1.2 2.2 0.0
v 1.5 2.8 0.0
e

etc., where each line starting with a v records an x,y vertex in a line drawing, and where each new continuous
line has an n tag above and an e tag below it. Note that every z value is automatically 0.0. Successive lines
are drawn in red, green, blue to show them as separate entities.

After drawing is complete, pushing "s" key while drawing window is in focus will save a file named 
drawing_hh_mm_ss.obj with the time of save encoded.

The drawing saves in the sketch's folder; command-k or Sketch->Show Sketch Folder will reveal the folder
in the operating system if you're having trouble finding it.

*/

int maxX = 20;
int maxY = 20;

color red = color(255, 0, 0);
color green = color(0, 255, 0);
color blue = color(0, 0, 255);
int colorflag = 0;

boolean firstline = true;

String[] lines = {""};

void setup() {
  size(2000, 2000);
  fill(red);
  lines = append(lines, "# team A1 painting robot OBJ file");
  lines = append(lines, "# drawing started " + nf(hour(), 2) + ":"+nf(minute(), 2)+":"+nf(second(), 2)+" on "+
    month()+"/"+day()+"/"+year()+"\n\n\n");
}

void draw() {
  if (colorflag == 0) stroke(red);
  else if (colorflag == 1) stroke(blue);
  else stroke(green);
}

void mouseDragged() {
  if (firstline) lines = append(lines, "n");

  line(pmouseX, pmouseY, mouseX, mouseY);
  if (pmouseX != mouseX || pmouseY != mouseY) {
    double newx = map(mouseX+250, 0, width, 0, maxX)  -10;
    double newy = map(mouseY+250, 0, height, maxY, 0) -10;
    String vertexEntry = "v " + newx + " " + newy + " 0.0";
    lines = append(lines, vertexEntry);
  }
  
  firstline = false;
}

void mouseReleased() {
  lines = append(lines, "e\n\n");
  firstline = true;
  colorflag ++;
  colorflag = colorflag % 3;
}

void keyPressed() {
  if (key == 'p') print(lines);

  if (key == 's') saveStrings("drawing_"+nf(hour(), 2)+"_"+nf(minute(), 2)+"_"+nf(second(), 2)+".obj", lines);
}