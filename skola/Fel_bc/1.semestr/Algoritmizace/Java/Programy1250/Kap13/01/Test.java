/* Soubor Kap13\01\Test.java
 * Uk�zka pou�it� t��dy File. Metoda vytvorNovyPrazdnySoubor()
 * dostane jm�no souboru (pokud neobsahuje cestu, pak jde o soubor
 * v aktu�ln�m adres��i).
 * Metoda si nejprve vytvo�� instanci t��dy File a jej�mu konstruktoru
 * p�ed� dan� jm�no. Pak zjist�, zda dan� soubor existuje.
 * Pokud ano, sma�e ho. Pak vytvo�� nov�, pr�zdn�.
 * V p��pad� ne�sp�chu (nap�. pokud neexistuje adres��, ve kter�m
 * m� soubor b�t) vyvol� v�jimku.
 */

import java.io.*;
public class Test {

  public Test() {}
  public static File vytvorNovyPrazdnySoubor(String jmeno) throws java.io.IOException
  {
    File f = new File(jmeno);
    if(f.exists()) f.delete();
    f.createNewFile();
    return f;
  }
  public static void main(String[] a) throws java.io.IOException
  {
    String jmeno = "Data.dta";
    try {
      File f = vytvorNovyPrazdnySoubor(jmeno);
    }
    catch(java.io.IOException e)
    {
      System.out.print("Nepodarilo se vytvorit soubor " + jmeno);
      System.exit(1);
    }
  }
}