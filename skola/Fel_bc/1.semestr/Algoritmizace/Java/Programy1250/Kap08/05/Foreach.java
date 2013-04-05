// Soubor Kap08\05\Foreach.java
// P��klad pou�it� nov�ho tvaru p��kazu for
// ke zpracov�n� v�ech prvk� kontejneru

// Jen v JDK 5

import java.util.ArrayList;

public class Foreach
{
  // V�pis pole pomoc� for-each
  public static void vypisPole(int[] a)
  {
     for(long x: a)
       System.out.println(x);
  }

  // V�pis kontejneru pomoc� for-each
  public static void vypisKontejneru(ArrayList a)
  {
     for(Object x: a)
       System.out.println(x);
  }
  
  /* M��e nahradit ob� p�edchoz� metody

  public static void vypisKontejneru(Iterable a)
  {
     for(Object x: a)
       System.out.println(x);
  }
 */

  public static void main(String[] arg)
  {
     int[] A = {1, 2, 3};
     vypisPole(A);

     ArrayList al = new ArrayList();
     for(int i = 0; i < 10; i++) al.add(i);
     vypisKontejneru(al);
  }
}