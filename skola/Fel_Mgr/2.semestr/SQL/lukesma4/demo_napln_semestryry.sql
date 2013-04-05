create or replace
PROCEDURE DEMO_NAPLN_SEMESTRY AS 
-- created - Sobota-�nor-12-2011 Ivan Halaska
-- dalsi upravy:
begin
            declare V_POM PLS_INTEGER;
            begin select COUNT(*) into V_POM from DEMO_SEMESTR;
                  if V_POM > 0 then DBMS_OUTPUT.PUT_LINE('Tabulka DEMO_SEMESTR nen� pr�zdn�, kon��m');  
                  return;
                  end if;
            end;
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(091, 'Zimn� semestr 2009/10');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(101, 'Zimn� semestr 2010/11');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(111, 'Zimn� semestr 2011/12');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(121, 'Zimn� semestr 2012/13');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(131, 'Zimn� semestr 2013/14');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(141, 'Zimn� semestr 2014/15');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(151, 'Zimn� semestr 2015/16');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(161, 'Zimn� semestr 2016/17');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(171, 'Zimn� semestr 2017/18');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(181, 'Zimn� semestr 2018/19');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(191, 'Zimn� semestr 2019/20');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(201, 'Zimn� semestr 2020/21');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(211, 'Zimn� semestr 2021/22');
-- letni semestry
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(092, 'Letn� semestr 2009/10');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(102, 'Letn� semestr 2010/11');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(112, 'Letn� semestr 2011/12');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(122, 'Letn� semestr 2012/13');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(132, 'Letn� semestr 2013/14');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(142, 'Letn� semestr 2014/15');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(152, 'Letn� semestr 2015/16');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(162, 'Letn� semestr 2016/17');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(172, 'Letn� semestr 2017/18');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(182, 'Letn� semestr 2018/19');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(192, 'Letn� semestr 2019/20');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(202, 'Letn� semestr 2020/21');
INSERT INTO  DEMO_SEMESTR(SEMESTR_ID, NAZEV_SEMESTRU) VALUES(212, 'Letn� semestr 2021/22');
commit;
declare 
V_POM PLS_INTEGER; 
begin
select COUNT(*) into V_POM from DEMO_SEMESTR;
DBMS_OUTPUT.PUT_LINE('Do tabulky DEMO_SEMESTR bylo vlo�eno '||V_POM||' ��dk�');
end;
END DEMO_NAPLN_SEMESTRY;