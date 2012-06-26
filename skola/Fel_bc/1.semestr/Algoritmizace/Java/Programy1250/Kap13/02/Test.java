

/* Soubor Kap13\02\Test.java
 * Uk�zka �ten� a z�pisu dat primitivn�ch typ�.
 * Metoda zapisPoleDoSouboru(int[] a, String jmeno) 
 * pou�ije jako podkladov� proud FileOutputStream, jako filtr
 * pou�ije DataOutputStream. K v�pisu pole a do souboru jmeno
 * pou�ije metodu writeInt.
 * Metoda nactiPoleZeSouboru je podobn�, jen pou��v� proudy 
 * FileInputStream a DataInputStream a metodu readInt.
 */

import java.io.*;
public class Test {

  public Test() {}

  public static void zapisPoleDoSouboru(int[] a, String jmeno)
  throws java.io.IOException
  {
    File f = new File(jmeno);
    if(!f.exists()) f.createNewFile();

    FileOutputStream fos = new FileOutputStream(f);
    DataOutputStream vystup = new DataOutputStream(fos);
    for(int i = 0; i < a.length; i++)
      vystup.writeInt(a[i]);
    vystup.close();
  }

  public static void nactiPoleZeSouboru(int[] a, String jmeno)
  throws java.io.IOException
  {
    final int SIZE_OF_INT = 4;
    File f = new File(jmeno);
    if(!f.exists()) throw new java.io.IOException();
    int delka = (int)f.length()/SIZE_OF_INT;
    if(delka > a.length) delka = a.length;

    FileInputStream fis = new FileInputStream(f);
    DataInputStream vstup = new DataInputStream(fis);

    for(int i = 0, j; i < delka; i++)
      a[i] = vstup.readInt();
  }

  public static void main(String[] a) throws java.io.IOException
  {
    int[] p = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int[] q = new int[5];
    String jmeno = "Data.dta";

    zapisPoleDoSouboru(p, jmeno);

    nactiPoleZeSouboru(q, jmeno);

    for(int i = 0; i < q.length; i++) System.out.print(q[i]+" ");
  }
}