package alg2;
public class JednucheDatoveTypy {
    public static void main(String[] args ) {
        System.out.println(">>>  START # 1  \n");
        
int i=7;		// deklarace prom�nn� i typu int
double x;	// deklarace prom�nn� x typu double
boolean b;      // deklarace prom�nn� b typu boolean\
String s;       // deklarace prom�nn� b typu boolean\
final int XXX=64646383;
final int MAX = 100;
final String NAZEV_PREDMETU = "Algoritmizace";


i=55;
x=4512e11;
b=true;
s= "ALFA";
System.out.println("i = " + i*5);
System.out.println("x = " +x);
System.out.println("b = " +b);
System.out.println("s = " +s);
// .......................
int a, c;
a = 7;            //m� hodnotu 7 a lze tedy op�t p�i�adit!!
c = a = a + 6;    // vyhodnot� se jako y = (x = (x + 6)); 
System.out.println("a, c = " + a + " , " + c);        
//.........................

a=1; 
c=2;
System.out.println(a+c);
System.out.println("sou�et je "+(a+c));
System.out.println("sou�et je "+a+c);

System.out.println(">>>  STOP");
    }
}
