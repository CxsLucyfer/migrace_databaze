loadAndNormalizeData;

% Recode the output variable (origin) to 
% 1 - american cars
% 2 - non-american cars
auta.Var11 ~= 1;

% Split data to 1/4 for training and 3/4 for testing
[indtren, indtest] = splitData(nAuta, .25);
auta_tren = auta(indtren,:);
auta_test = auta(indtest,:);

% Prepare the features used for classification
auta_tren_in = [auta_tren.Var1 auta_tren.Var3]; 
auta_test_in = [auta_test.Var1 auta_test.Var3];

% Train the KNN classifier
model = trainClassKNN( auta_tren_in, auta_tren.Var11, 1);

% Plot the data and the classification areas
figure; hold on;
pareas(gcf, model);
% Training data
indAmer = (auta_tren.Var11 == 1);
indNAmer = (auta_tren.Var11 ~= 1);
plot(auta_tren.Var1(indAmer), auta_tren.Var3(indAmer), 'bo');
plot(auta_tren.Var1(indNAmer), auta_tren.Var3(indNAmer), 'ro');
pause;
% Testing data
indAmer = (auta_test.Var11 == 1);
indNAmer = (auta_test.Var11 ~= 1);
plot(auta_test.Var1(indAmer), auta_test.Var3(indAmer), 'bx');
plot(auta_test.Var1(indNAmer), auta_test.Var3(indNAmer), 'rx');
