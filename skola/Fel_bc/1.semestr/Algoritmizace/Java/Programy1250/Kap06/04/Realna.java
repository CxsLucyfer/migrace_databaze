/* Soubor Realna.java
   Chybn� porovn�n� re�ln�ch ��sel
   Program skon�� v nekone�n�m cyklu a 
   je t�eba ho zastavit Ctrl+C
*/

public class Realna
{
  public static void main(String [] s)
  {
    double d = 0.0;
    while(d !=  1.0)   // Zde je zakop�m pes
   {
 	System.out.println(d);
	d += 0.1;
   }
  }
}