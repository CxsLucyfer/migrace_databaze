// Soubor Kap12\01\Pokus.java
// Uk�zka pou�it� �ol�ku
// jako typov�ho parametru.

import java.util.*;

public class Pokus
{

 // Univerz�ln� metoda pro tisk jak�koli kolekce
 // Verze pro star�� JDK
 public static void tiskPoStaru(java.util.Collection c)
 {
  java.util.Iterator it = c.iterator(); // Z�sk�me iter�tor
  while(it.hasNext())                   // a projdeme kolekci
    System.out.println(it.next());      // prvek po prvku
 }
 

/* Nefunguje -- kolekce parametrizovan� typem Object 
   je prost� jin� typ ne� kolekce parametrizovan� nap�. 
   typem String
 */
/*
 public static void tisk(Collection<Object> c)
 {
   for(Object x: c) 
     System.out.println(x);
 }
*/

 // Univerz�ln� metoda pro tisk jak�koli kolekce
 // Verze pro JDK 5
 public static void tisk(Collection<?> c)
 {
   for(Object x: c) 
     System.out.println(x);
 }


 public static void main(String [] s)
 {
   ArrayList<String> als = new ArrayList<String>();
   als.add("Ahoj");
   als.add("lidi");
   //tiskPoStaru(als);  // To lze
   tisk(als);
 }
}
