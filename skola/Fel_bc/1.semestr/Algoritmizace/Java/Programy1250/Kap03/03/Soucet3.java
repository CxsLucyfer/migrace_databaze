/* S��t�n� dvou ��sel - p�et� pokus
   pou�iteln� pouze v JDK 5, nebo�
   ve star��ch verz�ch chyb� metoda printf
   soubor kap03\04\Soucet3.java
*/

public class Soucet3 {
  public static void main(String[] arg){
    int i = 12; // Prvn� s��tanec
    int j = 25; // Druh� s��tanec
    int k = i+j;// Sou�et
    System.out.printf("Soucet cisel %d + %d je %d", i, j, k);
  }
};