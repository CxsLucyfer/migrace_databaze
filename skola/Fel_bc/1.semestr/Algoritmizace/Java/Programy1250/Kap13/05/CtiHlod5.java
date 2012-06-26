/* Soubor Kap13\06\CtiHlod.java
 * P�e�te �esk� text v k�dov�n� Latin 2 ze souboru a vyp�e na konzolu.
 * P�edpokl�d� existenci souboru data.dta v aktu�ln�m adres��i, kter� obsahuje
 * �esk� text v k�dov�n� Latin 2 (CP 852).
 * Jako vstup lze pou��t nap�. soubor vytvo�en� programem Hlod.java, 
 * kter� najdete v tomto adres��i.
 */

// Pouze pro JDK 5 a vy���

import java.io.*;
import java.util.Scanner;

public class CtiHlod5 {

  public static void main(String[] args) throws Exception{
    File f = new File("data.dta");
    if(!f.exists()) throw new IOException("soubor neexistuje");

    Scanner skan = new Scanner(f, "Cp852");

    OutputStreamWriter osw = new OutputStreamWriter(System.out, "Cp852");
    PrintWriter pw = new PrintWriter(osw);

    while(skan.hasNextLine())
    {
      pw.println(skan.nextLine());
    }
    pw.close();
    skan.close();
  }
}