/* S��t�n� dvou ��sel - t�et� pokus

*  Vyu��v� MojeIO.inInt()
*  Do adres��e s programem (nebo do kter�hokoli
*  adres��e, do kter�ho ukazuje prom�nn� CLASSPATH,
*  je t�eba nakop�rovat soubor MojeIO.class.

*  soubor kap03\04\Soucet4.java
*/

public class Soucet4 {
  public static void main(String[] arg){
    System.out.print("Zadej prvni scitanec: ");
    int i = MojeIO.inInt(); // �ti prvn� scitanec
    System.out.print("Zadej druhy scitanec: ");
    int j = MojeIO.inInt(); // �ti druh� s��tanec
    System.out.println("Jejich soucet je " + (i+j));
  }
};