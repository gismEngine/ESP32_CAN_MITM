var rpm = 0;
var turbo = 0;

var headerY = [];
var headerX = [];

var colHighlight0 = 1;
var colHighlight1 = 1;

var rowHighlight0 = 1;
var rowHighlight1 = 1;		

function initTable(){
	var hRow = document.querySelectorAll('#tbl th');
	for (var i = 0; i < hRow.length; i++) {
		var cell = hRow[i];
		headerX.push(parseFloat(cell.innerText.trim()));
	}

	var rows = document.querySelectorAll('#tbl tr');
	for (var i = 1; i < rows.length; i++) {
		var row = rows[i];
		var cells = row.getElementsByTagName('td');
		
		headerY.push(parseFloat(cells.item(0).innerText.trim()));
	}

	console.log(headerX);
	console.log(headerY);
}

function newInputRpmMap(newValue, valueId, units){
	rpm = newValue;
	document.getElementById(valueId).innerHTML = newValue + units;
	
	// Hightlight MAP column
	var rangeFound = false;
	for (var i = 0; i < headerX.length - 1; i++) {
		if(newValue == headerX[i]){
		
			colHighlight0 = i;
			colHighlight1 = i;
			
			rangeFound = true;
			break;
		}else if((newValue > headerX[i]) && (newValue < headerX[ i + 1])){
		
			colHighlight0 = i;
			colHighlight1 = i + 1;

			rangeFound = true;
			break;
		}
	}
	
	if(newValue >= headerX[headerX.length - 1]){
		
		colHighlight0 = headerX.length - 1;
		colHighlight1 = headerX.length - 1;
			
			
	}else if(newValue < headerX[0]){
	
		colHighlight0 = 0;
		colHighlight1 = 0;

	}
	
	highlightCrossTable();
}


function newInputTurboMap(newValue, valueId, units){
	turbo = newValue;
	document.getElementById(valueId).innerHTML = newValue + units;
	
	// Hightlight MAP column
	var rangeFound = false;
	for (var i = 0; i < headerY.length - 1; i++) {
		if(newValue == headerY[i]){
		
			rowHighlight0 = i + 1;
			rowHighlight1 = i + 1;
			
			rangeFound = true;
			break;
		}else if((newValue > headerY[i]) && (newValue < headerY[ i + 1])){
		
			rowHighlight0 = i + 1;
			rowHighlight1 = i + 2;

			rangeFound = true;
			break;
		}
	}
	
	if(newValue >= headerY[headerY.length - 1]){
		
		rowHighlight0 = headerY.length;
		rowHighlight1 = headerY.length;
			
			
	}else if(newValue < headerY[0]){
	
		rowHighlight0 = 1;
		rowHighlight1 = 1;

	}
	
	highlightCrossTable();			
}


function highlightCrossTable(){
	
	clearTableBackground(); 							// Clear table background
	
	highlightColTableBackground(colHighlight0);
	highlightColTableBackground(colHighlight1);
	
	highlightRowTableBackground(rowHighlight0);
	highlightRowTableBackground(rowHighlight1);
	
	highlightCellBackground(colHighlight0, rowHighlight0);
	highlightCellBackground(colHighlight0, rowHighlight1);
	highlightCellBackground(colHighlight1, rowHighlight0);
	highlightCellBackground(colHighlight1, rowHighlight1);
	
	
	document.getElementById('interpolate').innerHTML = 'Interpolate: ' + interpolateFull().toFixed(2);
}

function interpolateFull(){
		
	x_0 = headerX[colHighlight0];
	x_1 = headerX[colHighlight1];
	
	//console.log(x_0, x_1);
	
	y_00 = getTableValue(colHighlight0, rowHighlight0);
	y_01 = getTableValue(colHighlight1, rowHighlight0);
	
	//console.log(y_00, y_01);
	
	var y_int0 = interpolateSimp(x_0, y_00, x_1, y_01, rpm);
	
	//console.log(y_int0);
	
	y_10 = getTableValue(colHighlight0, rowHighlight1);
	y_11 = getTableValue(colHighlight1, rowHighlight1);
	
	//console.log(y_10, y_11);
	
	var y_int1 = interpolateSimp(x_0, y_10, x_1, y_11, rpm);
	
	//console.log(y_int1);
	
	y_0 = headerY[rowHighlight0 - 1];
	y_1 = headerY[rowHighlight1 - 1];
	
	//console.log(y_0, y_1);
	
	
	var intValue = interpolateSimp(y_0, y_int0, y_1, y_int1, turbo);
	
	//console.log(intValue);
	return intValue;
}

function interpolateSimp(x0, y0, x1, y1, x){
	if (x1 == x0) return ((y0 + y1) / 2);
	return (x - x0) * (y1 - y0) / (x1 - x0) + y0;
}

function getTableValue(col, row){
	if (col == 0) return;
	if (row == 0) return;

	var rows = document.querySelectorAll('#tbl tr');
	var hRow = rows.item(row);
	
	var cells = hRow.getElementsByTagName('td');
	
	cells.item(col).style.backgroundColor= '#ffbf00'
	
	return parseFloat(cells.item(col).innerText.trim());
}

function highlightCellBackground(col, row){
	if (col == 0) return;
	if (row == 0) return;

	var rows = document.querySelectorAll('#tbl tr');
	var hRow = rows.item(row);
	
	var cells = hRow.getElementsByTagName('td');
	
	cells.item(col).style.backgroundColor= '#ffbf00'
}

function highlightColTableBackground(col){
	var rows = document.querySelectorAll('#tbl tr');
	for (var i = 1; i < rows.length; i++) {
		var row = rows[i];
		var cells = row.getElementsByTagName('td');
		
		var cell = cells.item(col)
		cell.style.backgroundColor= '#ebf1f2'
	}
}

function highlightRowTableBackground(row){		
	
	var rows = document.querySelectorAll('#tbl tr');		
	var r = rows.item(row);
	var cells = r.getElementsByTagName('td');
	
	for (var i = 1; i < cells.length; i++) {
		cells[i].style.backgroundColor= '#ebf1f2'
	}
}
	
function clearTableBackground(){
	var rows = document.querySelectorAll('#tbl tr');
	for (var i = 0; i < rows.length; i++) {
		var row = rows[i];
		var cells = row.getElementsByTagName('td');
		
		for (var j = 1; j < cells.length; j++) {
			var cell = cells[j];
			cell.style.backgroundColor= '#fafcfc'
		}
	}
}