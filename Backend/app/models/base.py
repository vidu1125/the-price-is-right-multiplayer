from sqlalchemy.orm import DeclarativeBase
from sqlalchemy import inspect

class Base(DeclarativeBase):
    def to_dict(self):
        return {
            c.key: getattr(self, c.key)
            for c in inspect(self).mapper.column_attrs
        }
