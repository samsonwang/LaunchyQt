
#include "InputDataList.h"
#include "GlobalVar.h"

void InputDataList::parse(const QString& text) {
	if (text.isEmpty()) {
		clear();
	}
	else {
		QStringList split = text.split(QString(" ") + QChar(0x25ba) + " ");

		// Truncate inputData to the same length as the new input text
		if (split.count() < count()) {
			erase(begin() + split.count(), end());
		}

		// Truncate to the first different entry
		for (int i = 0; i < count(); ++i) {
			(*this)[i].removeLabel(LABEL_HISTORY);
			if (at(i).getText() != split[i]) {
				erase(begin() + i, end());
				break;
			}
		}

		// And add anything new
		for (int i = count(); i < split.count(); i++) {
			InputData data(split[i]);
			push_back(data);
		}
	}
}


QString InputDataList::toString(bool omitLast) const {
	QString result = "";
	for (int i = 0; i < count(); ++i) {
		if (i > 0)
			result += QString(" ") + QChar(0x25ba) + " ";
		if (!omitLast || i < count()-1)
			result += at(i).getText();
	}
	return result;
}
