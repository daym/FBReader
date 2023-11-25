#include <vector>
#include "PdfObject.h"
#include "PdfImageRasterizer.h"
#include "PdfImage.h"

#ifdef USE_CAIRO
#include <cairo.h>

PdfImageRasterizer::PdfImageRasterizer(std::map<shared_ptr<PdfObject>, shared_ptr<PdfFont> >& fontsByName) :
	ZLSingleImage("image/auto"),
	myToUnicodeTable(NULL),
	myFontsByName(fontsByName)
{
	// TODO draw into a SVG surface or cairo_recording_surface_create (1.9.4), then find out the extremes and scale down to howevermuch space we really need.
	int width = 400; // FIXME
	int height = 300; // FIXME
	mySurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	myContext = cairo_create(mySurface);
	/*
	cairo_translate
	cairo_rotate
	cairo_scale
	cairo_transform
	cairo_set_matrix
	cairo_get_matrix
	cairo_identity_matrix
	cairo_device_to_user
	cairo_user_to_device
	*/
}

void PdfImageRasterizer::transform(double x, double y, double& tx, double& ty) {
	// FIXME scale down to surface extents.
	tx = x;
	ty = y;
}

void PdfImageRasterizer::setNonstrokingColor(std::vector<double> color) {
	if(color.size() == 3) {
		// FIXME distinguish from the stroking color?
		cairo_set_source_rgb(myContext, color[0], color[1], color[2]);
	} else {
		std::cerr << "warning: opColorSetNonstrokingRGBColor: did not get 3 arguments." << std::endl;
	}
}

void PdfImageRasterizer::processInstruction(const PdfInstruction& instruction) {
	PdfOperator operator_ = instruction.operator_();
	switch(operator_) {
	case opColorSetNonstrokingRGBColor:
		setNonstrokingColor(instruction.extractFloatArguments());
		break;
	case opClipIntersect:
		cairo_clip(myContext);
		break;
	case opPaintNoPaint:
		// FIXME cairo_close_path???(myContext);
		break;
	case opPathBegin:
		cairo_new_path(myContext);
		if(instruction.extractTwoFloatArguments(x, y)) {
			transform(x, y, tx, ty);
			cairo_move_to(myContext, tx, ty);
		}
		break;
	case opPathAddLine:
		if(instruction.extractTwoFloatArguments(x, y)) {
			transform(x, y, tx, ty);
			cairo_line_to(myContext, tx, ty);
		}
		break;
	case opPaintFillEvenOdd:
		cairo_set_fill_rule(myContext, CAIRO_FILL_RULE_EVEN_ODD);
		cairo_fill(myContext);
		break;
	case opTransformationMatrixAppend:
		appendTransformationMatrix(instruction.extractFloatArguments());
		break;
	case opPushGraphicsState:
		cairo_save(myContext);
		break;
	case opPopGraphicsState:
		cairo_restore(myContext);
		break;
	case opSetTextMatrix:
	case opSetTextLeading: // FIXME can also be outside
	case opSetFontAndSize: // FIXME can also be outside
	case opShowStringWithVariableSpacing:
	case opSetWordSpacing: // FIXME can also be outside
	case opSetCharacterSpacing: // FIXME can also be outside
	case opMoveCaret:
	case opMoveCaretToStartOfNextLine:
	case opMoveCaretToStartOfNextLineAndOffsetAndSetLeading:
		std::cerr << "warning: ignored text instruction in non-text block." << std::endl;
		break;
	case opInvalid:
	default:
		std::cerr << "warning: ignored unknown operator." << std::endl;
	}
}

void PdfImageRasterizer::appendTransformationMatrix(std::vector<double> components) {
	if(components.size() == 6) {
		cairo_matrix_t matrix;
		cairo_matrix_init(&matrix, components[0], components[1], components[2], components[3], components[4], components[5]);
		cairo_transform(myContext, &matrix);
		// TODO make this pushable.
	} else {
		std::cerr << "warning: ignored invalid transformation matrix." << std::endl;
	}
}

void PdfImageRasterizer::processInstructions(const std::vector<shared_ptr<PdfInstruction> >& instructions) {
	for(std::vector<shared_ptr<PdfInstruction> >::const_iterator iter_instructions = instructions.begin(); iter_instructions != instructions.end(); ++iter_instructions) {
		shared_ptr<PdfInstruction> instruction = *iter_instructions;
		if(instruction->operator_() == opBeginText) {
			const shared_ptr<PdfArrayObject>& arguments = instruction->arguments();
			int argument_count = arguments->size();
			if(argument_count > 0) {
				shared_ptr<PdfObject> item = (*arguments)[0];
				if(item->type() == PdfObject::CONTENT) {
					const std::vector<shared_ptr<PdfInstruction> >& x_instructions = ((PdfContent&)(*item)).instructions();
					processTextInstructions(x_instructions);
				}
			}
		} else
			processInstruction(*instruction);
	}
}

const shared_ptr<std::string> PdfImageRasterizer::stringData() const {
	std::stringstream result(std::ios_base::out | std::ios_base::binary);
	int width = cairo_image_surface_get_width(mySurface);
	int height = cairo_image_surface_get_height(mySurface);
	cairo_format_t format = cairo_image_surface_get_format(mySurface);
	wrapTGA(width, height, 8, (format == CAIRO_FORMAT_ARGB32 || format == CAIRO_FORMAT_RGB24) ? 4 : 1, result);
	result << cairo_image_surface_get_data(mySurface);
	return new std::string(result.str());
}

void PdfImageRasterizer::processTextInstructions(const std::vector<shared_ptr<PdfInstruction> >& instructions) {
	// TODO: its own mini-world.
	for(std::vector<shared_ptr<PdfInstruction> >::const_iterator iter_instructions = instructions.begin(); iter_instructions != instructions.end(); ++iter_instructions) {
		shared_ptr<PdfInstruction> instruction = *iter_instructions;
		processTextInstruction(*instruction);
	}
}

void PdfImageRasterizer::processTextInstruction(const PdfInstruction& instruction) {
	// TODO.
	PdfOperator operator_ = instruction.operator_();
	switch(operator_) {
	case opSetTextMatrix:
		break;
	case opSetTextLeading:
		break;
	case opSetFontAndSize:
		break;
	case opSetWordSpacing:
		break;
	case opSetCharacterSpacing:
		break;
	case opMoveCaret:
		break;
	case opMoveCaretToStartOfNextLine:
		break;
	case opMoveCaretToStartOfNextLineAndOffsetAndSetLeading:
		break;
	case opShowStringWithVariableSpacing:
	case opShowString:
	case opNextLineShowString:
	case opNextLineSpacedShowString:
		showString(instruction.extractFirstArgument());
		break;
	case opPathBegin:
	case opPathAddLine:
	case opPathAddCubicBezier123:
	case opPathAddCubicBezier23:
	case opPathAddCubicBezier13:
	case opPathClose:
	case opPathRectangle:
	case opPaintStroke:
	case opPaintCloseAndStroke:
	case opPaintCloseAndFill:
	case opPaintFillEvenOdd:
	case opPaintFillAndStroke:
	case opPaintFillAndStrokeEvenOdd:
	case opPaintCloseAndFillAndStroke:
	case opPaintCloseAndFillAndStrokeEvenOdd:
	case opPaintNoPaint:
	case opPaintExternalObject:
	case opClipIntersect:
	case opClipIntersectEvenOdd:
	case opSetLineWidth:
	case opSetLineCap:
	case opSetLineJoin:
	case opSetMiterLimit:
	case opSetDashPattern:
	case opSetColorIntent:
	case opFlatness:
	case opSetParameterValue:
	case opSetMarkedContentPoint:
	case opSetMarkedContentPointWithAttributes:
	case opInvalid: /*?*/
		std::cerr << "warning: ignored non-text operator in text block." << std::endl;
		break;
	}
}

static std::string mangleTextPart(const std::string& value) {
	return value;
}

void PdfImageRasterizer::showString(shared_ptr<PdfObject> item) {
	std::string result;
	if(item->type() == PdfObject::ARRAY) {
		PdfArrayObject* xarray = (PdfArrayObject*)&*item;
		int argument_count = xarray->size();
		for(int i = 0; i < argument_count; ++i) {
			shared_ptr<PdfObject> xitem = (*xarray)[i];
			if(xitem->type() == PdfObject::STRING) { // esp. ! number
				PdfStringObject* stringObject = (PdfStringObject*)&*xitem;
				result = result + mangleTextPart(myToUnicodeTable->convertStringToUnicode(stringObject->value()));
				//result = result + ' ';
			} else if(xitem->type() == PdfObject::INTEGER_NUMBER) {
				PdfIntegerObject* integerObject = (PdfIntegerObject*)&*xitem;
				if(integerObject->value() < -57) // TODO these are textspace/1000.
				result = result + " ";
			} else if(xitem->type() == PdfObject::REAL_NUMBER) {
				PdfRealObject* realObject = (PdfRealObject*)&*xitem;
				if(realObject->value() < -57)
					result = result + " ";
				/*debugging: else
					result = result + "@";*/
			}
		};
		//result = Copy(result, 1, Length(result) - 1);
	} else if(item->type() == PdfObject::STRING) {
		PdfStringObject* stringObject = (PdfStringObject*)&*item;
		result = result + myToUnicodeTable->convertStringToUnicode(stringObject->value()); 
	}
	cairo_show_text(myContext, result.c_str());
}

#endif /* def USE_CAIRO */
