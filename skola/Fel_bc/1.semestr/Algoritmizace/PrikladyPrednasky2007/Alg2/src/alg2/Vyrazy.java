package alg2;



public class Vyrazy {
  public static void main(String[] args) {
   System.out.println("7/3="+7/3);

   System.out.println("7%3="+7%3);


    int i=10; double x=12.3; boolean b;
   System.out.println("i==10 ~ " + (i==10));             // vyp�e se true
   System.out.println("i+1!=10 ~ " + (i+1!=10));           // vyp�e se true
    b = i>x;
   System.out.println("b ~ " + b);                 // vyp�e se false
//........................
    int n = 10; boolean b1 = false, b2 = true; 
    double y, z;
   System.out.println(1<=n && n<=20);	// vyp�e se true
   System.out.println(b1 || !b2);		// vyp�e se false
   y=0; x=1;z=2;
   System.out.println(y != 0 && x/y < z);		// vyp�e se false


  }
}