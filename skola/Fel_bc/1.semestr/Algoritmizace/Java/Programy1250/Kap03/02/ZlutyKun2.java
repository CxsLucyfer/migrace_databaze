/* soubor Kap03\02\ZlutyKun2.java
 * V�stup �e�tiny do konzolov�ho okna
 * spr�vn�.
 * Poznamenejme, �e v rozs�hlej��ch programech
 * bude t�eba p�ipojit za p��kaz
 * p.println("�lu�ou�k� ...");
 * je�t� p��kaz
 * p.flush();
 */

import java.io.*;

/** Jedin� t��da prvn�ho programu */
public class ZlutyKun2 {
  /** Jedin� metoda. P�edstavuje cel� program. */
  public static void main(String[] arg) throws Exception
  {
    OutputStreamWriter osw = 
    new OutputStreamWriter(System.out, "Cp852");
     PrintWriter p = new PrintWriter(osw);
    p.println("�lu�ou�k� k�� p��ern� �p�l ��belsk� �dy.");
    p.close();
  }
}